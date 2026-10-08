#include "game/pch.h"
#include "game/objects/player_visual.h"

#include "game/objects/weapon.h"

#include <limits>

using namespace labrador;
using namespace mattmath;

PlayerVisual::PlayerVisual(const WorkshopPlayerPresentation* presentation,
    RenderResources* render_resources, PlayerVisualMode mode, int view_index) :
    TextureObject(presentation->sprite_sheet, presentation->pixel_frame,
        render_resources), presentation_(presentation), mode_(mode)
{
    if (view_index < 0 || view_index >= 4 || mode == PlayerVisualMode::original)
    {
        throw std::invalid_argument("Workshop player visual needs a comparison mode and view 0..3");
    }
    size_t index = 0;
    for (const std::string& name : presentation->idle_frames)
    {
        this->body_frames_[index++] = this->resolve_pair(name, name + "_team");
    }
    for (const std::string& name : presentation->run_frames)
    {
        this->body_frames_[index++] = this->resolve_pair(name, name + "_team");
    }
    this->body_frames_[index++] = this->resolve_pair(presentation->air_frame,
        presentation->air_frame + "_team");
    this->body_frames_[index] = this->resolve_pair(presentation->landing_frame,
        presentation->landing_frame + "_team");
    this->sprayer_ = { &presentation->sprayer,
        this->resolve_pair(presentation->sprayer.frame, presentation->sprayer.team_frame) };
    this->roller_ = { &presentation->roller,
        this->resolve_pair(presentation->roller.frame, presentation->roller.team_frame) };
    for (size_t pose = 0; pose < this->sprayer_poses_.size(); ++pose)
    {
        const WorkshopToolPresentation& info = presentation->sprayer_poses[pose];
        this->sprayer_poses_[pose] = { &info, this->resolve_pair(info.frame, info.team_frame) };
    }
    this->glove_frame_ = this->sprite_sheet()->resolve_sprite_frame(presentation->glove_frame);
    this->badge_frame_ = this->sprite_sheet()->resolve_sprite_frame(
        presentation->badge_frames[static_cast<size_t>(view_index)]);

    // Distribute quiet idle motion across the roster without consuming gameplay RNG.
    this->idle_phase_offset_ = static_cast<float>(view_index) /
        static_cast<float>(presentation->badge_frames.size()) *
        presentation->idle_frame_time * static_cast<float>(presentation->idle_frames.size());
}

PlayerVisual::FramePair PlayerVisual::resolve_pair(const std::string& frame,
    const std::string& team) const
{
    return { this->sprite_sheet()->resolve_sprite_frame(frame),
        this->sprite_sheet()->resolve_sprite_frame(team) };
}

void PlayerVisual::update(float dt, bool grounded, bool moving)
{
    this->landing_remaining_ = std::max(0.0f, this->landing_remaining_ - dt);
    if (grounded != this->was_grounded_ || moving != this->was_moving_)
    {
        this->animation_time_ = 0.0f;
    }
    this->was_grounded_ = grounded;
    this->was_moving_ = moving;
    if (!grounded)
    {
        this->body_frame_index_ = 6;
        this->landing_remaining_ = 0.0f;
        return;
    }
    if (moving)
    {
        // New movement interrupts the decorative landing recovery.
        this->landing_remaining_ = 0.0f;
    }
    if (this->landing_remaining_ > 0.0f)
    {
        this->body_frame_index_ = 7;
        return;
    }
    const float frame_time = moving ? this->presentation_->run_frame_time :
        this->presentation_->idle_frame_time;
    const size_t count = moving ? this->presentation_->run_frames.size() :
        this->presentation_->idle_frames.size();
    this->animation_time_ = std::fmod(this->animation_time_ + dt,
        frame_time * static_cast<float>(count));
    const float phase = std::fmod(this->animation_time_ +
        (moving ? 0.0f : this->idle_phase_offset_), frame_time * static_cast<float>(count));
    this->body_frame_index_ = (moving ? 2U : 0U) + std::min(count - 1,
        static_cast<size_t>(phase / frame_time));
}

void PlayerVisual::land()
{
    this->landing_remaining_ = this->presentation_->landing_duration;
    this->body_frame_index_ = 7;
    this->animation_time_ = 0.0f;
}

void PlayerVisual::takeoff()
{
    this->landing_remaining_ = 0.0f;
    this->body_frame_index_ = 6;
    this->animation_time_ = 0.0f;
}

void PlayerVisual::reset()
{
    this->body_frame_index_ = 6;
    this->animation_time_ = 0.0f;
    this->landing_remaining_ = 0.0f;
    this->was_grounded_ = false;
    this->was_moving_ = false;
}

size_t PlayerVisual::body_frame_index() const { return this->body_frame_index_; }
float PlayerVisual::animation_time() const { return this->animation_time_; }
PlayerVisualMode PlayerVisual::mode() const { return this->mode_; }

PlayerVisual::Attachment PlayerVisual::attachment(const Weapon& weapon) const
{
    Attachment result;
    result.muzzle = weapon.nozzle_pos();
    result.grip = weapon.draw_pos();
    result.support_grip = result.grip;
    const float aim = weapon.rotation();
    const bool left = aim > PI / 2.0f || aim < -PI / 2.0f;
    if (weapon.type() == WeaponType::sprayer)
    {
        if (this->mode_ == PlayerVisualMode::continuous)
        {
            result.tool = &this->sprayer_;
            result.rotation = aim;
            result.flip = left ? SpriteFlip::vertical : SpriteFlip::none;
        }
        else
        {
            // Select artwork only. The actual weapon angle and projectile
            // trajectory remain continuous between the five authored poses.
            const float right_half_aim = std::atan2(std::sin(aim), std::abs(std::cos(aim)));
            float nearest = std::numeric_limits<float>::max();
            for (const Tool& pose : this->sprayer_poses_)
            {
                const float distance = std::abs(right_half_aim - pose.presentation->aim_angle);
                if (distance < nearest)
                {
                    nearest = distance;
                    result.tool = &pose;
                }
            }
            result.flip = left ? SpriteFlip::horizontal : SpriteFlip::none;
        }
    }
    else if (weapon.type() == WeaponType::roller)
    {
        result.tool = &this->roller_;
        result.flip = left ? SpriteFlip::horizontal : SpriteFlip::none;
        // A newly built weapon has its initial angle until the first active
        // tick. Honor that pose during the countdown too. Normal roller updates
        // use the authored fixed angle, so their correction is exactly zero.
        const float authored_angle = left ? PI - this->presentation_->roller.aim_angle :
            this->presentation_->roller.aim_angle;
        result.rotation = aim - authored_angle;
    }
    if (result.tool != nullptr)
    {
        const auto transform = [&result](Vector2F offset)
        {
            if (result.flip == SpriteFlip::horizontal) { offset.x = -offset.x; }
            if (result.flip == SpriteFlip::vertical) { offset.y = -offset.y; }
            return Vector2F::rotate_vector(offset, result.rotation);
        };
        // Translate the selected pose to the real muzzle. Merely quantising its
        // rotation around the old grip would put paint outside the visible tip.
        result.grip = result.muzzle - transform(result.tool->presentation->muzzle_offset);
        result.support_grip = result.grip + transform(
            result.tool->presentation->support_grip_offset);
    }
    return result;
}

RectangleF PlayerVisual::body_rectangle(const RectangleF& collision_rectangle) const
{
    const Vector2F size = this->presentation_->body_size;
    return RectangleF(collision_rectangle.center().x - size.x / 2.0f,
        collision_rectangle.bottom() - size.y, size.x, size.y);
}

RectangleF PlayerVisual::badge_rectangle(const RectangleF& body) const
{
    const Vector2F center(body.center().x + this->presentation_->badge_offset.x,
        body.top() + this->presentation_->badge_offset.y);
    return RectangleF(center - this->presentation_->badge_size / 2.0f,
        this->presentation_->badge_size);
}

void PlayerVisual::draw_frame(DrawList& draw_list, SpriteSheet::frame_handle frame,
    const Vector2F& position, const Vector2F& size, const Colour& colour,
    SpriteFlip flip, float rotation) const
{
    const SpriteFrame& sprite = this->sprite_sheet()->sprite_frame(frame);
    Vector2F correction;
    // UV mirroring does not mirror the sprite origin. Supply the difference so
    // an off-centre authored grip stays attached for both facings.
    if (flip == SpriteFlip::horizontal)
    {
        correction.x = static_cast<float>(sprite.source_rectangle().width) -
            sprite.origin().x * 2.0f;
    }
    if (flip == SpriteFlip::vertical)
    {
        correction.y = static_cast<float>(sprite.source_rectangle().height) -
            sprite.origin().y * 2.0f;
    }
    this->draw_with(draw_list, RectangleF(position, size), frame, colour,
        correction, flip, rotation);
}

void PlayerVisual::draw_pair(DrawList& draw_list, const FramePair& frames,
    const Vector2F& position, const Vector2F& size, const Colour& team_colour,
    SpriteFlip flip, float rotation) const
{
    this->draw_frame(draw_list, frames.base, position, size, Colour::white, flip, rotation);
    this->draw_frame(draw_list, frames.team, position, size, team_colour, flip, rotation);
}

void PlayerVisual::draw_segment(DrawList& draw_list, const Vector2F& start,
    const Vector2F& finish, const Vector2F& grid_origin, float width,
    const Colour& colour) const
{
    const float pixel = this->presentation_->pixel_size;
    const Vector2F delta = finish - start;
    const float step_count = std::ceil(std::max(std::abs(delta.x), std::abs(delta.y)) / pixel);
    // Bounded cosmetic work even if content supplies an extreme offset or tiny
    // texel size. Check before converting a potentially huge float to int.
    constexpr int maximum_segment_steps = 256;
    if (!std::isfinite(step_count))
    {
        throw std::runtime_error("Workshop arm segment has non-finite geometry");
    }
    const int steps = std::max(1, static_cast<int>(std::min(step_count,
        static_cast<float>(maximum_segment_steps))));
    for (int step = 0; step <= steps; ++step)
    {
        const Vector2F point = start + delta * (static_cast<float>(step) /
            static_cast<float>(steps));
        const Vector2F snapped(
            grid_origin.x + std::round((point.x - grid_origin.x) / pixel) * pixel,
            grid_origin.y + std::round((point.y - grid_origin.y) / pixel) * pixel);
        this->draw_with(draw_list, RectangleF(snapped - Vector2F(width / 2.0f),
            Vector2F(width)), this->frame(), colour, Vector2F::ZERO,
            SpriteFlip::none, 0.0f);
    }
}

void PlayerVisual::draw_arm(DrawList& draw_list, const Vector2F& shoulder,
    const Vector2F& grip, const Vector2F& grid_origin) const
{
    const Vector2F elbow = (shoulder + grip) / 2.0f +
        Vector2F(0.0f, this->presentation_->elbow_drop);
    for (const bool outline : { true, false })
    {
        const float width = this->presentation_->sleeve_width +
            (outline ? this->presentation_->arm_outline_width * 2.0f : 0.0f);
        const Colour& colour = outline ? this->presentation_->arm_outline_colour :
            this->presentation_->sleeve_colour;
        this->draw_segment(draw_list, shoulder, elbow, grid_origin, width, colour);
        this->draw_segment(draw_list, elbow, grip, grid_origin, width, colour);
    }
}

void PlayerVisual::draw(DrawList& draw_list, const RectangleF& collision_rectangle,
    bool facing_right, const Colour& team_colour, const Weapon& weapon, bool debug) const
{
    const RectangleF body = this->body_rectangle(collision_rectangle);
    const Attachment held = this->attachment(weapon);
    const float facing = facing_right ? 1.0f : -1.0f;
    const auto shoulder = [&body, facing](Vector2F offset)
    {
        offset.x *= facing;
        return body.center() + offset;
    };
    this->draw_arm(draw_list, shoulder(this->presentation_->rear_shoulder_offset),
        held.support_grip, body.position());
    this->draw_pair(draw_list, this->body_frames_[this->body_frame_index_],
        body.center(), body.size(), team_colour,
        facing_right ? SpriteFlip::none : SpriteFlip::horizontal, 0.0f);
    this->draw_arm(draw_list, shoulder(this->presentation_->shoulder_offset),
        held.grip, body.position());
    if (held.tool != nullptr)
    {
        const Colour paint = weapon.type() == WeaponType::roller &&
            !weapon.shooting_this_update() ? Colour::white : team_colour;
        this->draw_pair(draw_list, held.tool->frames, held.grip,
            held.tool->presentation->size, paint, held.flip, held.rotation);
    }
    else
    {
        // The other three tools remain playable with their existing artwork.
        weapon.draw(draw_list);
    }
    this->draw_frame(draw_list, this->glove_frame_, held.support_grip,
        this->presentation_->glove_size, Colour::white, SpriteFlip::none, 0.0f);
    this->draw_frame(draw_list, this->glove_frame_, held.grip,
        this->presentation_->glove_size, Colour::white, SpriteFlip::none, 0.0f);
    const RectangleF badge = this->badge_rectangle(body);
    this->draw_frame(draw_list, this->badge_frame_, badge.center(), badge.size(),
        Colour::white, SpriteFlip::none, 0.0f);
    if (debug) { weapon.draw_nozzle_debug(draw_list); }
}

RectangleF PlayerVisual::bounds(const RectangleF& collision_rectangle,
    const Weapon& weapon) const
{
    const RectangleF body = this->body_rectangle(collision_rectangle);
    RectangleF result = RectangleF::union_of(body, this->badge_rectangle(body));
    const Attachment held = this->attachment(weapon);
    if (held.tool != nullptr)
    {
        // One diagonal around the grip covers every orientation, including the
        // pose translation and authored pivots close to a frame edge.
        const float radius = held.tool->presentation->size.length();
        result = RectangleF::union_of(result, RectangleF(held.grip - Vector2F(radius),
            Vector2F(radius * 2.0f)));
    }
    else
    {
        result = RectangleF::union_of(result, weapon.draw_bounds());
    }
    // Include the elbow and the pixel stroke/glove past either endpoint.
    const float margin = this->presentation_->elbow_drop +
        this->presentation_->sleeve_width + this->presentation_->arm_outline_width +
        this->presentation_->glove_size.length() + this->presentation_->pixel_size;
    return RectangleF(result.position() - Vector2F(margin),
        result.size() + Vector2F(margin * 2.0f));
}
