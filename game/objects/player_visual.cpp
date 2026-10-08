#include "game/pch.h"
#include "game/objects/player_visual.h"

#include "game/objects/weapon.h"

#include <cmath>

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

void PlayerVisual::update_pose(const Weapon& weapon)
{
    if (this->mode_ != PlayerVisualMode::authored || weapon.type() != WeaponType::sprayer)
    {
        return;
    }
    this->sprayer_pose_ = this->choose_sprayer_pose(weapon.rotation(), this->sprayer_pose_);
}

void PlayerVisual::reset()
{
    this->sprayer_pose_.reset();
    this->body_frame_index_ = 6;
    this->animation_time_ = 0.0f;
    this->landing_remaining_ = 0.0f;
    this->was_grounded_ = false;
    this->was_moving_ = false;
}

size_t PlayerVisual::body_frame_index() const { return this->body_frame_index_; }
float PlayerVisual::animation_time() const { return this->animation_time_; }
PlayerVisualMode PlayerVisual::mode() const { return this->mode_; }

size_t PlayerVisual::sprayer_pose_index(const Weapon& weapon) const
{
    return this->sprayer_pose_.has_value() ? *this->sprayer_pose_ :
        this->choose_sprayer_pose(weapon.rotation(), std::nullopt);
}

size_t PlayerVisual::choose_sprayer_pose(float aim, std::optional<size_t> held) const
{
    // Select artwork only. The actual weapon angle and projectile trajectory
    // remain continuous between the five authored poses.
    const float right_half_aim = std::atan2(std::sin(aim), std::abs(std::cos(aim)));
    const auto distance = [this, right_half_aim](size_t pose)
    {
        return std::abs(right_half_aim - this->presentation_->sprayer_poses[pose].aim_angle);
    };
    size_t nearest = 0;
    for (size_t pose = 1; pose < this->sprayer_poses_.size(); ++pose)
    {
        if (distance(pose) < distance(nearest)) { nearest = pose; }
    }
    // Between neighbours the distance difference is twice the overshoot past
    // their midpoint, so keep the held pose until the aim is a band beyond it.
    if (held.has_value() && *held != nearest &&
        distance(*held) - distance(nearest) <= this->presentation_->pose_hysteresis * 2.0f)
    {
        return *held;
    }
    return nearest;
}

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
            result.tool = &this->sprayer_poses_[this->sprayer_pose_index(weapon)];
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

RectangleF PlayerVisual::badge_rectangle(const RectangleF& body, bool facing_right) const
{
    // Authored for a right-facing body; mirroring keeps it toward the back.
    const float facing = facing_right ? 1.0f : -1.0f;
    const Vector2F center(body.center().x + this->presentation_->badge_offset.x * facing,
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
    const Colour& colour, bool include_start) const
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
    for (int step = include_start ? 0 : 1; step <= steps; ++step)
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

Vector2F PlayerVisual::elbow(const Vector2F& shoulder, const Vector2F& grip,
    float facing, const Vector2F& grid_origin) const
{
    // Two equal bones: the elbow sags off the span's midpoint by what the bone
    // length leaves over, capped by elbow_drop, so a long reach straightens.
    const Vector2F span = grip - shoulder;
    const float length = span.length();
    const float half_bone = this->presentation_->arm_length / 2.0f;
    float sag = std::min(this->presentation_->elbow_drop, std::sqrt(
        std::max(0.0f, half_bone * half_bone - length * length / 4.0f)));
    Vector2F normal(0.0f, 1.0f);
    if (length > 0.001f)
    {
        normal = Vector2F(-span.y / length, span.x / length);
        // Bend downward and slightly toward the back. Within a pose the short
        // front span can sweep through any direction, so whichever rule picks
        // the side must flip somewhere; fade the sag to nothing as the choice
        // nears its tie so the arm straightens through the flip, not jumps.
        constexpr float back_bias = 0.25f;
        constexpr float fade_band = 0.3f;
        const float preference = normal.y - back_bias * facing * normal.x;
        if (preference < 0.0f) { normal = -normal; }
        sag *= std::min(1.0f, std::abs(preference) / fade_band);
    }
    const Vector2F joint = (shoulder + grip) / 2.0f + normal * sag;
    // Snap once so both segments meet on one shared joint texel.
    const float pixel = this->presentation_->pixel_size;
    return Vector2F(
        grid_origin.x + std::round((joint.x - grid_origin.x) / pixel) * pixel,
        grid_origin.y + std::round((joint.y - grid_origin.y) / pixel) * pixel);
}

std::array<PlayerVisual::ArmPose, 2> PlayerVisual::arms(const RectangleF& body,
    bool facing_right, const Attachment& held) const
{
    const float facing = facing_right ? 1.0f : -1.0f;
    const auto arm = [&](Vector2F shoulder_offset, const Vector2F& grip)
    {
        shoulder_offset.x *= facing;
        const Vector2F shoulder = body.center() + shoulder_offset;
        return ArmPose{ shoulder,
            this->elbow(shoulder, grip, facing, body.position()), grip };
    };
    return { arm(this->presentation_->rear_shoulder_offset, held.support_grip),
        arm(this->presentation_->shoulder_offset, held.grip) };
}

std::array<PlayerVisual::ArmPose, 2> PlayerVisual::arms(
    const RectangleF& collision_rectangle, bool facing_right, const Weapon& weapon) const
{
    return this->arms(this->body_rectangle(collision_rectangle), facing_right,
        this->attachment(weapon));
}

RectangleF PlayerVisual::badge_bounds(const RectangleF& collision_rectangle,
    bool facing_right) const
{
    return this->badge_rectangle(this->body_rectangle(collision_rectangle), facing_right);
}

void PlayerVisual::draw_arm(DrawList& draw_list, const ArmPose& arm,
    const Vector2F& grid_origin) const
{
    for (const bool outline : { true, false })
    {
        const float width = this->presentation_->sleeve_width +
            (outline ? this->presentation_->arm_outline_width * 2.0f : 0.0f);
        const Colour& colour = outline ? this->presentation_->arm_outline_colour :
            this->presentation_->sleeve_colour;
        this->draw_segment(draw_list, arm.shoulder, arm.elbow, grid_origin, width, colour, true);
        this->draw_segment(draw_list, arm.elbow, arm.grip, grid_origin, width, colour, false);
    }
}

float PlayerVisual::relative_luminance(const Colour& colour)
{
    // WCAG 2 relative luminance of the encoded (sRGB) channels.
    const auto linear = [](float channel)
    {
        return channel <= 0.04045f ? channel / 12.92f :
            std::pow((channel + 0.055f) / 1.055f, 2.4f);
    };
    return 0.2126f * linear(colour.r) + 0.7152f * linear(colour.g) +
        0.0722f * linear(colour.b);
}

Colour PlayerVisual::mask_tint(const Colour& team_colour, float minimum_luminance)
{
    if (minimum_luminance <= 0.0f || relative_luminance(team_colour) >= minimum_luminance)
    {
        return team_colour;
    }
    const auto mix = [&team_colour](float amount)
    {
        return Colour(team_colour.r + (1.0f - team_colour.r) * amount,
            team_colour.g + (1.0f - team_colour.g) * amount,
            team_colour.b + (1.0f - team_colour.b) * amount, team_colour.a);
    };
    // Luminance rises monotonically toward white; bisect for the least mix.
    float low = 0.0f;
    float high = 1.0f;
    for (int iteration = 0; iteration < 24; ++iteration)
    {
        const float middle = (low + high) / 2.0f;
        if (relative_luminance(mix(middle)) >= minimum_luminance) { high = middle; }
        else { low = middle; }
    }
    return mix(high);
}

void PlayerVisual::draw(DrawList& draw_list, const RectangleF& collision_rectangle,
    bool facing_right, const Colour& team_colour, const Weapon& weapon, bool debug) const
{
    const RectangleF body = this->body_rectangle(collision_rectangle);
    const Attachment held = this->attachment(weapon);
    const std::array<ArmPose, 2> arms = this->arms(body, facing_right, held);
    // Only the workshop masks are lifted; paint, HUD and the original visual
    // keep the true team colour.
    const Colour mask = mask_tint(team_colour,
        this->presentation_->team_mask_minimum_luminance);
    this->draw_arm(draw_list, arms[0], body.position());
    this->draw_pair(draw_list, this->body_frames_[this->body_frame_index_],
        body.center(), body.size(), mask,
        facing_right ? SpriteFlip::none : SpriteFlip::horizontal, 0.0f);
    this->draw_arm(draw_list, arms[1], body.position());
    // Before the tool, so an upward muzzle is never hidden behind the badge.
    const RectangleF badge = this->badge_rectangle(body, facing_right);
    this->draw_frame(draw_list, this->badge_frame_, badge.center(), badge.size(),
        Colour::white, SpriteFlip::none, 0.0f);
    if (held.tool != nullptr)
    {
        const Colour paint = weapon.type() == WeaponType::roller &&
            !weapon.shooting_this_update() ? Colour::white : mask;
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
    if (debug) { weapon.draw_nozzle_debug(draw_list); }
}

RectangleF PlayerVisual::bounds(const RectangleF& collision_rectangle,
    bool facing_right, const Weapon& weapon) const
{
    const RectangleF body = this->body_rectangle(collision_rectangle);
    RectangleF result = RectangleF::union_of(body, this->badge_rectangle(body, facing_right));
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
    for (const ArmPose& arm : this->arms(body, facing_right, held))
    {
        for (const Vector2F& joint : { arm.shoulder, arm.elbow, arm.grip })
        {
            result = RectangleF::union_of(result, RectangleF(joint, Vector2F::ZERO));
        }
    }
    // Include the pixel stroke and glove past every joint.
    const float margin = this->presentation_->sleeve_width +
        this->presentation_->arm_outline_width +
        this->presentation_->glove_size.length() + this->presentation_->pixel_size;
    return RectangleF(result.position() - Vector2F(margin),
        result.size() + Vector2F(margin * 2.0f));
}
