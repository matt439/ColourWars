#include "game/pch.h"
#include "game/objects/weapon_feedback.h"

#include <cmath>

using namespace labrador;
using namespace mattmath;

WeaponFeedback::WeaponFeedback(const WeaponFeedbackPresentation* presentation,
    RenderResources* render_resources) :
    TextureObject(presentation->sprite_sheet, presentation->fill_frame, render_resources),
    presentation_(presentation)
{
}

void WeaponFeedback::update(float dt)
{
    if (dt <= 0.0f || !std::isfinite(dt)) { return; }
    if (!this->presentation_->enabled)
    {
        this->clear();
        return;
    }
    this->state_.muzzle_remaining = std::max(0.0f, this->state_.muzzle_remaining - dt);
    this->state_.empty_remaining = std::max(0.0f, this->state_.empty_remaining - dt);
    this->state_.recovered_remaining = std::max(0.0f, this->state_.recovered_remaining - dt);
}

void WeaponFeedback::shot(WeaponType type)
{
    if (!this->presentation_->enabled) { return; }
    this->state_.empty_remaining = 0.0f;
    // The roller already paints at its supported contact. Streams finish each
    // pulse before a later paid shot begins the next, rather than pinning it.
    if (type != WeaponType::roller && this->state_.muzzle_remaining <= 0.0f)
    {
        this->state_.muzzle_remaining = this->presentation_->muzzle_duration;
    }
}

void WeaponFeedback::empty()
{
    if (!this->presentation_->enabled) { return; }
    this->state_.muzzle_remaining = 0.0f;
    this->state_.recovered_remaining = 0.0f;
    this->state_.empty_remaining = this->presentation_->empty_duration;
}

void WeaponFeedback::recovered()
{
    if (!this->presentation_->enabled) { return; }
    this->state_.empty_remaining = 0.0f;
    this->state_.recovered_remaining = this->presentation_->recovered_duration;
}

void WeaponFeedback::clear() { this->state_ = {}; }
const WeaponFeedbackState& WeaponFeedback::state() const { return this->state_; }

void WeaponFeedback::draw_block(DrawList& draw_list, const Vector2F& center,
    const Vector2F& size, float angle, const Colour& colour, float alpha) const
{
    const RectangleI& source = this->sprite_sheet()->sprite_frame(this->frame()).source_rectangle();
    const Vector2F origin(static_cast<float>(source.width) * 0.5f,
        static_cast<float>(source.height) * 0.5f);
    const float border = this->presentation_->keyline_width * 2.0f;
    this->draw_with(draw_list, RectangleF(center, size + Vector2F(border, border)),
        this->frame(), this->presentation_->keyline * alpha, origin, SpriteFlip::none, angle);
    this->draw_with(draw_list, RectangleF(center, size), this->frame(), colour * alpha,
        origin, SpriteFlip::none, angle);
}

void WeaponFeedback::draw(DrawList& draw_list, const Vector2F& nozzle,
    const Vector2F& pivot, float angle, WeaponType type, const Colour& team_colour) const
{
    if (!this->presentation_->enabled) { return; }
    const auto& look = *this->presentation_;
    if (this->state_.muzzle_remaining > 0.0f && type != WeaponType::roller)
    {
        const float alpha = std::clamp(this->state_.muzzle_remaining / look.muzzle_duration, 0.0f, 1.0f);
        const float progress = 1.0f - alpha;
        const Vector2F size = look.muzzle_sizes.at(static_cast<size_t>(type));
        // These short, blunt paint shapes are attached to the continuous muzzle
        // transform. No random draws, launch offsets or aim quantisation.
        const auto mark = [&](float x, float y, float width, float height, float turn = 0.0f)
        {
            const Vector2F local((x + progress * 0.3f) * size.x, y * size.y);
            const Vector2F center = nozzle + Vector2F::rotate_vector(local, angle);
            this->draw_block(draw_list, center, Vector2F(width * size.x, height * size.y),
                angle + turn, team_colour, alpha);
        };
        switch (type)
        {
        case WeaponType::sprayer:
            mark(0.35f, 0.0f, 0.6f, 0.35f);
            mark(0.65f, -0.4f, 0.22f, 0.3f, -0.2f);
            mark(0.8f, 0.3f, 0.18f, 0.4f, 0.15f);
            break;
        case WeaponType::sniper:
            mark(0.55f, 0.0f, 0.9f, 0.25f);
            mark(0.3f, -0.55f, 0.35f, 0.25f, -0.3f);
            mark(0.3f, 0.55f, 0.35f, 0.25f, 0.3f);
            break;
        case WeaponType::mister:
            for (int index = -2; index <= 2; ++index)
            {
                mark(0.65f - static_cast<float>(std::abs(index)) * 0.15f,
                    static_cast<float>(index) * 0.25f, 0.16f, 0.16f);
            }
            break;
        case WeaponType::bucket:
            mark(0.3f, -0.3f, 0.3f, 0.4f, -0.15f);
            mark(0.7f, 0.0f, 0.4f, 0.3f, 0.15f);
            mark(0.45f, 0.4f, 0.25f, 0.3f);
            break;
        default:
            break;
        }
    }

    const Vector2F status = pivot - Vector2F(0.0f, look.status_offset);
    const float size = look.status_size;
    if (this->state_.empty_remaining > 0.0f)
    {
        const float alpha = std::clamp(this->state_.empty_remaining / look.empty_duration, 0.0f, 1.0f);
        // A dry white cross has no team-paint centre and is never a cooldown
        // warning. Its event shares the existing empty-fire repetition budget.
        for (const float turn : { -PI / 4.0f, PI / 4.0f })
        {
            this->draw_block(draw_list, status, Vector2F(size, size * 0.25f),
                turn, look.highlight, alpha);
        }
    }
    if (this->state_.recovered_remaining > 0.0f)
    {
        const float alpha = std::clamp(this->state_.recovered_remaining / look.recovered_duration, 0.0f, 1.0f);
        // A returning team-colour bead with two white rays acknowledges enough
        // paint for a whole shot. This is a brief event, not a readiness meter.
        this->draw_block(draw_list, status, Vector2F(size * 0.65f, size),
            0.0f, team_colour, alpha);
        for (const float side : { -1.0f, 1.0f })
        {
            this->draw_block(draw_list, status + Vector2F(side * size, 0.0f),
                Vector2F(size * 0.4f, size * 0.2f), 0.0f, look.highlight, alpha);
        }
    }
}

RectangleF WeaponFeedback::bounds(const Vector2F& nozzle, const Vector2F& pivot,
    WeaponType type) const
{
    RectangleF result(pivot, Vector2F::ZERO);
    if (!this->presentation_->enabled) { return result; }
    const auto& look = *this->presentation_;
    if (this->state_.muzzle_remaining > 0.0f && type != WeaponType::roller)
    {
        const float reach = look.muzzle_sizes.at(static_cast<size_t>(type)).length() * 2.0f +
            look.keyline_width * 2.0f;
        result = RectangleF::union_of(result, RectangleF(nozzle - Vector2F(reach, reach),
            Vector2F(reach * 2.0f, reach * 2.0f)));
    }
    if (this->state_.empty_remaining > 0.0f || this->state_.recovered_remaining > 0.0f)
    {
        const Vector2F status = pivot - Vector2F(0.0f, look.status_offset);
        const float reach = look.status_size * 1.5f + look.keyline_width * 2.0f;
        result = RectangleF::union_of(result, RectangleF(status - Vector2F(reach, reach),
            Vector2F(reach * 2.0f, reach * 2.0f)));
    }
    return result;
}
