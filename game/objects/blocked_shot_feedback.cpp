#include "game/pch.h"
#include "game/objects/blocked_shot_feedback.h"

#include <algorithm>
#include <cmath>

using namespace labrador;
using namespace mattmath;

BlockedShotFeedback::BlockedShotFeedback(const BlockedShotPresentation* presentation,
    RenderResources* render_resources) :
    TextureObject(presentation->sprite_sheet, presentation->fill_frame, render_resources),
    presentation_(presentation)
{
}

size_t BlockedShotFeedback::owner_index(int owner)
{
    return owner >= 0 && owner < paint_owners::count ?
        static_cast<size_t>(owner) : static_cast<size_t>(paint_owners::count);
}

void BlockedShotFeedback::update(float dt)
{
    // A paused or invalid step never rewinds, emits or poisons a snapshot.
    if (dt <= 0.0f || !std::isfinite(dt)) { return; }
    if (!this->presentation_->enabled)
    {
        this->clear();
        return;
    }
    size_t remaining = 0;
    for (const Cue& current : this->cues())
    {
        Cue cue = current;
        cue.age += dt;
        if (cue.age < this->presentation_->duration)
        {
            this->cues_[remaining++] = cue;
        }
    }
    this->count_ = remaining;
    for (float& timer : this->repeat_remaining_)
    {
        timer = std::max(0.0f, timer - dt);
    }
}

void BlockedShotFeedback::emit(const BlockedShotContact& contact)
{
    if (!this->presentation_->enabled) { return; }
    const Colour& colour = contact.team_colour;
    if (!std::isfinite(contact.position.x) || !std::isfinite(contact.position.y) ||
        !std::isfinite(contact.normal.x) || !std::isfinite(contact.normal.y) ||
        !std::isfinite(colour.r) || !std::isfinite(colour.g) ||
        !std::isfinite(colour.b) || !std::isfinite(colour.a))
    {
        return;
    }
    const float length = std::hypot(contact.normal.x, contact.normal.y);
    if (length <= 0.0f || !std::isfinite(length)) { return; }
    float& repeat = this->repeat_remaining_[owner_index(contact.player_num)];
    if (repeat > 0.0f) { return; }
    if (this->count_ == this->cues_.size())
    {
        // Retire the oldest cue under extreme repetition; storage never grows.
        std::move(this->cues_.begin() + 1, this->cues_.end(), this->cues_.begin());
        --this->count_;
    }
    BlockedShotContact resolved = contact;
    resolved.normal /= length;
    this->cues_[this->count_++] = { resolved, 0.0f };
    repeat = this->presentation_->repeat_delay;
}

void BlockedShotFeedback::clear()
{
    this->count_ = 0;
    this->repeat_remaining_.fill(0.0f);
}

std::span<const BlockedShotFeedback::Cue> BlockedShotFeedback::cues() const
{
    return { this->cues_.data(), this->count_ };
}

float BlockedShotFeedback::repeat_remaining(int owner) const
{
    return this->repeat_remaining_[owner_index(owner)];
}

void BlockedShotFeedback::draw_block(DrawList& draw_list, const Vector2F& center,
    const Vector2F& size, float angle, const Colour& colour, float alpha) const
{
    const RectangleI& source = this->sprite_sheet()->sprite_frame(this->frame()).source_rectangle();
    const Vector2F origin(static_cast<float>(source.width) * 0.5f,
        static_cast<float>(source.height) * 0.5f);
    const float edge = this->presentation_->keyline_width * 2.0f;
    this->draw_with(draw_list, RectangleF(center, size + Vector2F(edge, edge)),
        this->frame(), this->presentation_->keyline * alpha, origin, SpriteFlip::none, angle);
    this->draw_with(draw_list, RectangleF(center, size), this->frame(),
        colour * alpha, origin, SpriteFlip::none, angle);
}

void BlockedShotFeedback::draw(DrawList& draw_list) const
{
    if (!this->presentation_->enabled) { return; }
    const auto& look = *this->presentation_;
    for (const Cue& cue : this->cues())
    {
        const float progress = std::clamp(cue.age / look.duration, 0.0f, 1.0f);
        const float alpha = 1.0f - progress * progress;
        const Vector2F normal = cue.contact.normal;
        const Vector2F tangent(-normal.y, normal.x);
        const Vector2F point = cue.contact.position;
        const float angle = std::atan2(normal.y, normal.x);
        const float stroke = look.stroke_width;

        // A small white stop tick sits wholly outside the resolved face. Its
        // outward stem explains the stopped shot without painting that face.
        const Vector2F bar = point + normal * (look.keyline_width + stroke * 0.5f);
        this->draw_block(draw_list, bar, Vector2F(stroke, look.width), angle,
            look.highlight, alpha);
        this->draw_block(draw_list,
            point + normal * (look.keyline_width + stroke * 1.5f),
            Vector2F(stroke * 2.0f, stroke), angle, look.highlight, alpha);

        // Two uneven beads rebound away from the obstruction and separate
        // sideways. No settled mark or gameplay random draw is left behind.
        for (int index = 0; index < 2; ++index)
        {
            const float side = index == 0 ? -1.0f : 1.0f;
            const float scale = index == 0 ? 1.0f : 0.8f;
            const float distance = stroke + look.bead_size +
                look.reach * progress * (2.0f - progress) * scale;
            const float spread = side * look.width * (0.2f + progress * 0.25f);
            const Vector2F center = point + normal * distance + tangent * spread;
            const float size = look.bead_size * scale * (1.0f - progress * 0.25f);
            this->draw_block(draw_list, center, Vector2F(size, size * 0.8f),
                angle + side * 0.3f, cue.contact.team_colour, alpha);
        }
    }
}

RectangleF BlockedShotFeedback::cue_bounds(const Cue& cue) const
{
    const auto& look = *this->presentation_;
    // The full lifetime envelope includes rotated bead corners and outlines,
    // independent of age. Draws therefore cannot change the reported extent.
    const float outward = std::max(look.stroke_width * 2.5f,
        look.stroke_width + look.bead_size * 2.0f + look.reach) +
        look.keyline_width * 2.0f;
    const float sideways = look.width * 0.5f + look.bead_size +
        look.keyline_width * 2.0f;
    const Vector2F normal = cue.contact.normal;
    const Vector2F tangent(-normal.y, normal.x);
    const Vector2F center = cue.contact.position + normal * (outward * 0.5f);
    const Vector2F half_size(
        std::abs(normal.x) * outward * 0.5f + std::abs(tangent.x) * sideways,
        std::abs(normal.y) * outward * 0.5f + std::abs(tangent.y) * sideways);
    return RectangleF(center - half_size, half_size * 2.0f);
}

RectangleF BlockedShotFeedback::bounds() const
{
    if (!this->presentation_->enabled || this->count_ == 0) { return RectangleF::ZERO; }
    RectangleF result = this->cue_bounds(this->cues_[0]);
    for (size_t index = 1; index < this->count_; ++index)
    {
        result = RectangleF::union_of(result, this->cue_bounds(this->cues_[index]));
    }
    return result;
}
