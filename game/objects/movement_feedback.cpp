#include "game/pch.h"
#include "game/objects/movement_feedback.h"

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
        const float margin = contact ? 3.0f * burst.particle_size +
            this->presentation_->keyline_width + 2.0f * this->presentation_->pixel_size :
            2.0f * burst.particle_size + 2.0f * this->presentation_->pixel_size;
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
        this->draw_with(draw_list, rectangle, this->frame(), tint * fade,
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

void MovementFeedback::draw(DrawList& draw_list) const
{
    if (!this->presentation_->enabled) { return; }
    const float pixel = this->presentation_->pixel_size;
    const auto snap = [pixel](float value) { return std::round(value / pixel) * pixel; };
    for (const Effect& effect : this->effects())
    {
        if (effect.kind == MovementFeedbackKind::step || effect.kind == MovementFeedbackKind::skid)
        {
            this->draw_contact(draw_list, effect);
            continue;
        }
        const MovementBurstPresentation& burst = this->style(effect.kind);
        const float progress = effect.age / burst.duration;
        const float fade = 1.0f - progress;
        const float spread = burst.spread * effect.strength * progress;
        const float lift = burst.lift * effect.strength * progress * (2.0f - progress);
        const float size = std::max(pixel, snap(burst.particle_size *
            effect.strength * (1.0f - progress * 0.5f)));

        for (const float direction : { -1.0f, 1.0f })
        {
            const float x = snap(effect.position.x + direction * (size + spread));
            const float y = snap(effect.position.y - lift - size);
            const RectangleF puff(x - size / 2.0f, y, size, size);
            this->draw_with(draw_list, puff, this->frame(),
                this->presentation_->colour * fade, Vector2F::ZERO, SpriteFlip::none, 0.0f);
            const RectangleF fleck(x + direction * size, y - pixel,
                pixel, pixel);
            this->draw_with(draw_list, fleck, this->frame(),
                this->presentation_->highlight * fade, Vector2F::ZERO, SpriteFlip::none, 0.0f);
        }
    }
}
