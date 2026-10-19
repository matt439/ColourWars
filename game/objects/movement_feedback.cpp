#include "game/pch.h"
#include "game/objects/movement_feedback.h"
#include "game/objects/crude_drawing.h"

using namespace labrador;
using namespace mattmath;

MovementFeedback::MovementFeedback(const MovementFeedbackPresentation* presentation,
    RenderResources* render_resources) :
    TextureObject(presentation->sprite_sheet, presentation->fill_frame, render_resources),
    presentation_(presentation)
{
}

const MovementBurstPresentation& MovementFeedback::style(MovementFeedbackKind kind) const
{
    switch (kind)
    {
    case MovementFeedbackKind::takeoff: return this->presentation_->takeoff;
    case MovementFeedbackKind::landing: return this->presentation_->landing;
    case MovementFeedbackKind::step: return this->presentation_->step;
    case MovementFeedbackKind::skid: return this->presentation_->skid;
    }
    throw std::runtime_error("Invalid movement feedback kind");
}

void MovementFeedback::update(float dt)
{
    // Frozen snapshots cannot rewind or poison the shared effect ages.
    if (dt <= 0.0f || !std::isfinite(dt)) { return; }
    size_t remaining = 0;
    for (size_t index = 0; index < this->count_; ++index)
    {
        Effect effect = this->effects_[index];
        effect.age += dt;
        if (effect.age < this->style(effect.kind).duration)
        {
            this->effects_[remaining++] = effect;
        }
    }
    this->count_ = remaining;
}

void MovementFeedback::emit(MovementFeedbackKind kind, const Vector2F& position,
    float strength, float direction, std::optional<Colour> paint_colour)
{
    if (!this->presentation_->enabled) { return; }
    if (this->count_ == this->effects_.size())
    {
        // Retire the oldest burst under extreme repetition; never grow storage.
        std::move(this->effects_.begin() + 1, this->effects_.end(), this->effects_.begin());
        --this->count_;
    }
    this->effects_[this->count_++] = { kind, position, 0.0f, strength,
        direction, paint_colour };
}

void MovementFeedback::takeoff(const Vector2F& position)
{
    this->emit(MovementFeedbackKind::takeoff, position, 1.0f);
}

void MovementFeedback::land(const Vector2F& position, float speed)
{
    if (speed < this->presentation_->landing_min_speed) { return; }
    this->emit(MovementFeedbackKind::landing, position,
        std::clamp(speed / this->presentation_->full_impact_speed,
            this->presentation_->minimum_strength, 1.0f));
}

void MovementFeedback::contact(MovementFeedbackKind kind, const Vector2F& position,
    float direction, std::optional<Colour> paint_colour)
{
    if (kind != MovementFeedbackKind::step && kind != MovementFeedbackKind::skid)
    {
        return;
    }
    this->emit(kind, position, 1.0f, direction < 0.0f ? -1.0f : 1.0f, paint_colour);
}

void MovementFeedback::clear()
{
    this->count_ = 0;
}

std::span<const MovementFeedback::Effect> MovementFeedback::effects() const
{
    return { this->effects_.data(), this->count_ };
}

RectangleF MovementFeedback::bounds(const RectangleF& player_bounds) const
{
    RectangleF result = player_bounds;
    for (const Effect& effect : this->effects())
    {
        const MovementBurstPresentation& burst = this->style(effect.kind);
        // The outer fleck can reach two particle widths past the spread.
        // Include snapping in both axes so a neighbouring pane never culls it.
        const bool contact = effect.kind == MovementFeedbackKind::step ||
            effect.kind == MovementFeedbackKind::skid;
        const float margin = 3.0f * burst.particle_size +
            this->presentation_->keyline_width +
            (contact ? 2.0f : 3.0f) * this->presentation_->pixel_size;
        result = RectangleF::union_of(result, RectangleF(
            effect.position.x - burst.spread - margin,
            effect.position.y - burst.lift - margin,
            (burst.spread + margin) * 2.0f, burst.lift + margin * 2.0f));
    }
    return result;
}

void MovementFeedback::draw_contact(DrawList& draw_list, const Effect& effect) const
{
    const auto& look = *this->presentation_;
    const auto& burst = this->style(effect.kind);
    const float progress = effect.age / burst.duration;
    const float fade = 1.0f - progress;
    const float pixel = look.pixel_size;
    const auto snap = [pixel](float value) { return std::round(value / pixel) * pixel; };
    const Colour colour = effect.paint_colour.value_or(look.colour);
    const auto fill = [&](const RectangleF& rectangle, const Colour& tint)
    {
        this->draw_with(draw_list, rectangle, this->frame(), CrudeDrawing::premultiplied(tint) * fade,
            Vector2F::ZERO, SpriteFlip::none, 0.0f);
    };
    const auto border = [&](const RectangleF& rectangle)
    {
        const float edge = look.keyline_width;
        fill(RectangleF(rectangle.x - edge, rectangle.y - edge,
            rectangle.width + edge * 2.0f, rectangle.height + edge * 2.0f), look.keyline);
    };
    // Two footstep flecks or three skid flecks are the complete draw budget.
    // These authored offsets never consume the match's random stream.
    struct Fleck { float trail; float spread; float size; float rise; };
    constexpr std::array<Fleck, 3> flecks = {{
        { 0.5f, 0.35f, 1.0f, 0.55f },
        { 1.2f, 0.7f, 0.8f, 1.0f },
        { 1.8f, 1.0f, 0.6f, 0.7f },
    }};
    const size_t count = effect.kind == MovementFeedbackKind::skid ? 3 : 2;
    for (size_t index = 0; index < count; ++index)
    {
        const Fleck& fleck = flecks[index];
        const float size = std::max(pixel, snap(burst.particle_size * fleck.size *
            (1.0f - progress * 0.35f)));
        const float x = snap(effect.position.x - effect.direction *
            (burst.particle_size * fleck.trail + burst.spread * fleck.spread * progress));
        // Rise and settle back against the supporting face before fading out;
        // nothing remains on that face to imply new ownership or scoring.
        const float y = snap(effect.position.y - pixel - size * 0.25f -
            burst.lift * fleck.rise * 4.0f * progress * (1.0f - progress));
        const RectangleF main(x - size * 0.5f, y - size * 0.75f,
            size, size * 0.75f);
        const RectangleF corner(x, y - size * 0.45f,
            size * 0.65f, size * 0.45f);
        border(main);
        border(corner);
        fill(main, colour);
        fill(corner, colour);
        fill(RectangleF(main.x, main.y, std::min(pixel, size * 0.5f),
            std::min(pixel, main.height)), look.highlight);
    }
}

void MovementFeedback::draw_burst(DrawList& draw_list, const Effect& effect) const
{
    const MovementFeedbackPresentation& look = *this->presentation_;
    const MovementBurstPresentation& burst = this->style(effect.kind);
    const float progress = effect.age / burst.duration;
    const float fade = 1.0f - progress;
    const float spread = burst.spread * effect.strength * progress;
    const float lift = burst.lift * effect.strength * progress * (2.0f - progress);
    const float pixel = look.pixel_size;
    const auto snap = [pixel](float value) { return std::round(value / pixel) * pixel; };
    const float size = std::max(pixel, snap(burst.particle_size *
        effect.strength * (1.0f - progress * 0.5f)));
    std::array<RectangleF, 8> shape{};
    std::array<RectangleF, 2> highlights{};
    for (size_t side = 0; side < highlights.size(); ++side)
    {
        const float direction = side == 0 ? -1.0f : 1.0f;
        const float x = snap(effect.position.x + direction * (size + spread));
        const float y = snap(effect.position.y - lift - size);
        // Connected, stepped lobes: takeoff reaches upwards; landing spreads
        // low across the contact. Mirror the authored offsets, not the world
        // anchor. Partition their union below so faded joins never overdraw.
        const auto lobe = [&](float along, float down, float width, float height)
        {
            return RectangleF(x + direction * along * size - width * size * 0.5f,
                y + down * size, width * size, height * size);
        };
        const bool landing = effect.kind == MovementFeedbackKind::landing;
        const std::array<RectangleF, 3> lobes = landing ?
            std::array{ lobe(0.0f, 0.35f, 1.25f, 0.65f),
                lobe(-0.15f, 0.1f, 0.65f, 0.55f),
                lobe(0.55f, 0.6f, 0.65f, 0.4f) } :
            std::array{ lobe(0.0f, 0.2f, 0.9f, 0.8f),
                lobe(-0.15f, -0.15f, 0.55f, 0.65f),
                lobe(0.35f, 0.55f, 0.65f, 0.45f) };
        const RectangleF& crown = lobes[1];
        highlights[side] = RectangleF(crown.x + crown.width * 0.2f, crown.y + pixel * 0.25f,
            std::min(pixel, crown.width * 0.5f), std::min(pixel, crown.height * 0.5f));
        // One detached chip on each side keeps the contact lively without a
        // random stream or new particle storage.
        const float chip = std::max(pixel, snap(size * 0.2f));
        const RectangleF fleck(snap(x + direction * (size + chip) - chip * 0.5f),
            snap(y + (landing ? size * 0.45f : -pixel)), chip, chip);
        // Include the chip in the union, even when a wide keyline joins it to
        // the body. The neutral centre and outline must each blend only once.
        for (size_t index = 0; index < lobes.size(); ++index) { shape[side * 4 + index] = lobes[index]; }
        shape[side * 4 + 3] = fleck;
    }
    this->draw_puff(draw_list, shape, highlights, fade);
}

void MovementFeedback::draw_puff(DrawList& draw_list,
    const std::array<RectangleF, 8>& lobes,
    const std::array<RectangleF, 2>& highlights, float fade) const
{
    const MovementFeedbackPresentation& look = *this->presentation_;
    std::array<RectangleF, 8> outer = lobes;
    std::array<float, 36> x_cuts{};
    std::array<float, 36> y_cuts{};
    for (size_t index = 0; index < lobes.size(); ++index)
    {
        outer[index].inflate(look.keyline_width);
        x_cuts[index * 4] = lobes[index].left();
        x_cuts[index * 4 + 1] = lobes[index].right();
        x_cuts[index * 4 + 2] = outer[index].left();
        x_cuts[index * 4 + 3] = outer[index].right();
        y_cuts[index * 4] = lobes[index].top();
        y_cuts[index * 4 + 1] = lobes[index].bottom();
        y_cuts[index * 4 + 2] = outer[index].top();
        y_cuts[index * 4 + 3] = outer[index].bottom();
    }
    for (size_t index = 0; index < highlights.size(); ++index)
    {
        x_cuts[32 + index * 2] = highlights[index].left();
        x_cuts[33 + index * 2] = highlights[index].right();
        y_cuts[32 + index * 2] = highlights[index].top();
        y_cuts[33 + index * 2] = highlights[index].bottom();
    }
    std::sort(x_cuts.begin(), x_cuts.end());
    std::sort(y_cuts.begin(), y_cuts.end());
    const auto x_end = std::unique(x_cuts.begin(), x_cuts.end());
    const auto y_end = std::unique(y_cuts.begin(), y_cuts.end());

    // Split the outline, body and glint into disjoint horizontal spans, then
    // join vertical neighbours. Storage is bounded by the fixed authored lobes.
    // Every covered pixel blends once even midway through a translucent fade.
    struct Span { RectangleF rectangle; int ink; };
    std::array<Span, 1225> spans{}; // At most 35 by 35 cells between the cuts.
    size_t count = 0;
    const auto append = [&](float left, float right, float top, float bottom, int ink)
    {
        if (right <= left) { return; }
        for (size_t index = 0; index < count; ++index)
        {
            Span& span = spans[index];
            if (span.ink == ink && span.rectangle.left() == left &&
                span.rectangle.right() == right && span.rectangle.bottom() == top)
            {
                span.rectangle.height = bottom - span.rectangle.top();
                return;
            }
        }
        spans[count++] = { RectangleF(left, top, right - left, bottom - top), ink };
    };
    const auto contains = [](const RectangleF& rectangle, float x, float y)
    {
        return x > rectangle.left() && x < rectangle.right() &&
            y > rectangle.top() && y < rectangle.bottom();
    };
    for (auto row = y_cuts.begin(); row + 1 < y_end; ++row)
    {
        const float top = *row;
        const float bottom = *(row + 1);
        const float y = top + (bottom - top) * 0.5f;
        float left = x_cuts.front();
        int ink = -1;
        for (auto column = x_cuts.begin(); column + 1 < x_end; ++column)
        {
            const float x = *column + (*(column + 1) - *column) * 0.5f;
            const bool body = std::any_of(lobes.begin(), lobes.end(),
                [&](const RectangleF& rectangle) { return contains(rectangle, x, y); });
            const bool shell = std::any_of(outer.begin(), outer.end(),
                [&](const RectangleF& rectangle) { return contains(rectangle, x, y); });
            const bool glint = body && std::any_of(highlights.begin(), highlights.end(),
                [&](const RectangleF& rectangle) { return contains(rectangle, x, y); });
            const int next_ink = body ? (glint ? 2 : 1) :
                (shell ? 0 : -1);
            if (next_ink != ink)
            {
                if (ink >= 0) { append(left, *column, top, bottom, ink); }
                left = *column;
                ink = next_ink;
            }
        }
        if (ink >= 0) { append(left, *(x_end - 1), top, bottom, ink); }
    }
    const std::array colours{ look.keyline, look.colour, look.highlight };
    for (size_t index = 0; index < count; ++index)
    {
        const Span& span = spans[index];
        this->draw_with(draw_list, span.rectangle, this->frame(),
            CrudeDrawing::premultiplied(colours[static_cast<size_t>(span.ink)]) * fade,
            Vector2F::ZERO, SpriteFlip::none, 0.0f);
    }
}

void MovementFeedback::draw(DrawList& draw_list) const
{
    if (!this->presentation_->enabled) { return; }
    for (const Effect& effect : this->effects())
    {
        if (effect.kind == MovementFeedbackKind::step || effect.kind == MovementFeedbackKind::skid)
        {
            this->draw_contact(draw_list, effect);
        }
        else
        {
            this->draw_burst(draw_list, effect);
        }
    }
}
