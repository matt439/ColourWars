#include "game/pch.h"
#include "game/objects/combat_feedback.h"
#include "game/objects/crude_drawing.h"

#include <array>
#include <cmath>

using namespace labrador;
using namespace mattmath;

namespace
{
    // Authored directions and small shape differences, never gameplay random
    // draws. Eight fragments are the complete storage and rendering budget.
    struct Fragment
    {
        float x;
        float y;
        float size;
        bool tall;
    };

    constexpr std::array<Fragment, 8> fragments = {{
        { -1.0f, -0.65f, 1.0f, false },
        { -0.55f, -1.0f, 0.8f, true },
        { 0.3f, -0.9f, 1.15f, false },
        { 0.95f, -0.55f, 0.9f, true },
        { 1.0f, 0.25f, 1.1f, false },
        { 0.5f, 0.9f, 0.85f, true },
        { -0.35f, 1.0f, 1.0f, false },
        { -0.95f, 0.4f, 0.75f, true },
    }};

    RectangleF expanded(const RectangleF& rectangle, float margin)
    {
        return { rectangle.x - margin, rectangle.y - margin,
            rectangle.width + margin * 2.0f, rectangle.height + margin * 2.0f };
    }
}

CombatFeedback::CombatFeedback(const CombatFeedbackPresentation* presentation,
    RenderResources* render_resources) :
    TextureObject(presentation->sprite_sheet, presentation->fill_frame, render_resources),
    presentation_(presentation)
{
}

void CombatFeedback::update(float dt)
{
    // Paused/developer snapshots must neither rewind nor corrupt the timers.
    if (dt <= 0.0f || !std::isfinite(dt)) { return; }
    this->hurt_remaining_ = std::max(0.0f, this->hurt_remaining_ - dt);
    this->hurt_delay_remaining_ = std::max(0.0f, this->hurt_delay_remaining_ - dt);
    this->elimination_remaining_ = std::max(0.0f, this->elimination_remaining_ - dt);
}

bool CombatFeedback::hurt()
{
    if (!this->presentation_->enabled || this->hurt_delay_remaining_ > 0.0f ||
        this->hurt_remaining_ > 0.0f || this->elimination_remaining_ > 0.0f)
    {
        return false;
    }
    this->hurt_remaining_ = this->presentation_->hurt_duration;
    this->hurt_delay_remaining_ = this->presentation_->hurt_repeat_delay;
    return this->hurt_remaining_ > 0.0f;
}

void CombatFeedback::eliminate(const RectangleF& death_rectangle)
{
    this->clear();
    if (!this->presentation_->enabled) { return; }
    this->death_rectangle_ = death_rectangle;
    this->elimination_remaining_ = this->presentation_->elimination_duration;
}

void CombatFeedback::clear()
{
    this->hurt_remaining_ = 0.0f;
    this->hurt_delay_remaining_ = 0.0f;
    this->elimination_remaining_ = 0.0f;
    this->death_rectangle_ = RectangleF::ZERO;
}

float CombatFeedback::hurt_remaining() const { return this->hurt_remaining_; }
float CombatFeedback::elimination_remaining() const { return this->elimination_remaining_; }
const RectangleF& CombatFeedback::death_rectangle() const { return this->death_rectangle_; }

void CombatFeedback::draw_fill(DrawList& draw_list, const RectangleF& rectangle,
    const Colour& colour, float alpha) const
{
    this->draw_with(draw_list, rectangle, this->frame(), CrudeDrawing::premultiplied(colour) * alpha,
        Vector2F::ZERO, SpriteFlip::none, 0.0f);
}

void CombatFeedback::draw_keyline(DrawList& draw_list, const RectangleF& rectangle,
    float alpha) const
{
    this->draw_fill(draw_list, expanded(rectangle, this->presentation_->keyline_width),
        this->presentation_->keyline, alpha);
}

void CombatFeedback::draw(DrawList& draw_list, const RectangleF& current_body,
    const Colour& team_colour) const
{
    if (!this->presentation_->enabled) { return; }
    const auto& look = *this->presentation_;
    if (this->hurt_remaining_ > 0.0f)
    {
        const float alpha = std::clamp(this->hurt_remaining_ / look.hurt_duration, 0.0f, 1.0f);
        // Two short mouse-drawn ticks on each side of the head/upper torso.
        // Keep the face, team mask and aiming tool visible through the hit.
        for (const float direction : { -1.0f, 1.0f })
        {
            const float edge = direction < 0.0f ? current_body.left() : current_body.right();
            const float outer = direction < 0.0f ? edge - look.hurt_reach : edge;
            const RectangleF top(outer, current_body.top() + current_body.height * 0.12f,
                look.hurt_reach, look.hurt_width);
            const float short_reach = look.hurt_reach * 0.6f;
            const RectangleF lower(direction < 0.0f ? edge - short_reach : edge,
                current_body.top() + current_body.height * 0.34f,
                short_reach, look.hurt_width);
            this->draw_keyline(draw_list, top, alpha);
            this->draw_keyline(draw_list, lower, alpha);
            this->draw_fill(draw_list, top, look.highlight, alpha);
            this->draw_fill(draw_list, lower, look.highlight, alpha);
        }
    }
    if (this->elimination_remaining_ <= 0.0f) { return; }

    const float progress = std::clamp(1.0f -
        this->elimination_remaining_ / look.elimination_duration, 0.0f, 1.0f);
    const float alpha = 1.0f - progress;
    const Vector2F center = this->death_rectangle_.center();
    for (const Fragment& fragment : fragments)
    {
        const float size = look.particle_size * fragment.size;
        const float x = std::round(center.x + fragment.x *
            (this->death_rectangle_.width * 0.25f + look.elimination_spread * progress));
        const float y = std::round(center.y + fragment.y *
            (this->death_rectangle_.height * 0.25f + look.elimination_spread * progress) -
            look.elimination_lift * 4.0f * progress * (1.0f - progress));
        // Two joined blocks leave a blunt irregular corner rather than a
        // smooth particle. Draw both borders before either interior.
        const RectangleF main(x - size * 0.5f, y - size * 0.5f,
            size, size * (fragment.tall ? 1.25f : 0.75f));
        const RectangleF corner(x, y, size * 0.75f, size * 0.75f);
        this->draw_keyline(draw_list, main, alpha);
        this->draw_keyline(draw_list, corner, alpha);
        this->draw_fill(draw_list, main, team_colour, alpha);
        this->draw_fill(draw_list, corner, team_colour, alpha);
    }
}

RectangleF CombatFeedback::bounds(const RectangleF& current_body) const
{
    RectangleF result = current_body;
    if (!this->presentation_->enabled) { return result; }
    const auto& look = *this->presentation_;
    if (this->hurt_remaining_ > 0.0f)
    {
        result = RectangleF::union_of(result,
            expanded(current_body, look.hurt_reach + look.hurt_width + look.keyline_width));
    }
    if (this->elimination_remaining_ > 0.0f)
    {
        // Cover the complete remaining arc, including the initial body spread,
        // largest irregular block, its keyline and integer placement rounding.
        const float margin = look.elimination_spread + look.particle_size * 2.0f +
            look.keyline_width + 1.0f;
        const RectangleF burst(this->death_rectangle_.left() - margin,
            this->death_rectangle_.top() - margin - look.elimination_lift,
            this->death_rectangle_.width + margin * 2.0f,
            this->death_rectangle_.height + margin * 2.0f + look.elimination_lift);
        result = RectangleF::union_of(result, burst);
    }
    return result;
}
