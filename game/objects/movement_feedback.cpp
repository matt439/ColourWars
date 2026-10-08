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
    return kind == MovementFeedbackKind::takeoff ?
        this->presentation_->takeoff : this->presentation_->landing;
}

void MovementFeedback::update(float dt)
{
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
    float strength)
{
    if (!this->presentation_->enabled) { return; }
    if (this->count_ == this->effects_.size())
    {
        // Retire the oldest burst under extreme repetition; never grow storage.
        std::move(this->effects_.begin() + 1, this->effects_.end(), this->effects_.begin());
        --this->count_;
    }
    this->effects_[this->count_++] = { kind, position, 0.0f, strength };
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
        const float margin = 2.0f * burst.particle_size +
            2.0f * this->presentation_->pixel_size;
        result = RectangleF::union_of(result, RectangleF(
            effect.position.x - burst.spread - margin,
            effect.position.y - burst.lift - margin,
            (burst.spread + margin) * 2.0f, burst.lift + margin * 2.0f));
    }
    return result;
}

void MovementFeedback::draw(DrawList& draw_list) const
{
    if (!this->presentation_->enabled) { return; }
    const float pixel = this->presentation_->pixel_size;
    const auto snap = [pixel](float value) { return std::round(value / pixel) * pixel; };
    for (const Effect& effect : this->effects())
    {
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
