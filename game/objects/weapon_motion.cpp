#include "game/pch.h"
#include "game/objects/weapon.h"

using namespace mattmath;
using namespace labrador;

// Weapon's decorative tool motion: the pose state machine, its resets and the
// mist tail. Pose selection is drawn by Weapon::draw; the mist tail's extent
// is in Weapon::draw_bounds (both in weapon.cpp).

void Weapon::set_motion_presentation(const ToolMotionPresentation* presentation)
{
    this->motion_presentation_ = presentation;
    this->reset_mechanism_motion();
    if (presentation == nullptr) { return; }
    const std::array<std::string, 4> suffixes = { "", "_motion_1", "_motion_2", "_lift" };
    for (size_t index = 0; index < suffixes.size(); ++index)
    {
        const std::string name = this->details().frame_name + suffixes[index];
        this->motion_frames_[index] = this->sprite_sheet()->resolve_sprite_frame(name);
        this->motion_paint_frames_[index] = this->sprite_sheet()->resolve_sprite_frame(name + "_paint");
    }
    this->mist_sheet_ = this->render_resources()->resolve_sprite_sheet(presentation->mist_sheet);
    this->mist_frame_ = this->render_resources()->sprite_sheet(this->mist_sheet_)->
        resolve_sprite_frame(presentation->mist_frame);
}

void Weapon::reset_motion()
{
    if (this->feedback_ != nullptr) { this->feedback_->clear(); }
    this->reset_mechanism_motion();
}

void Weapon::reset_mechanism_motion()
{
    this->motion_ = {};
    this->motion_firing_ = false;
    this->motion_supported_ = false;
    this->mist_release_position_ = Vector2F::ZERO;
    this->mist_release_rotation_ = 0.0f;
}

const ToolMotionState& Weapon::motion_state() const
{
    return this->motion_;
}

int Weapon::visual_pose() const
{
    return static_cast<int>(this->motion_.pose);
}

void Weapon::update_motion(bool shot, float dt)
{
    if (dt <= 0.0f || this->motion_presentation_ == nullptr) { return; }
    if (!this->motion_presentation_->enabled)
    {
        this->reset_mechanism_motion();
        return;
    }
    const auto& look = *this->motion_presentation_;
    const auto& tool = look.tools.at(static_cast<size_t>(this->type_));
    auto& motion = this->motion_;
    // A decorative pressure gauge never finishes its recovery ahead of the
    // precision tool's actual cadence, even after a live balance edit.
    const float recovery = this->type_ == WeaponType::sniper ?
        std::max(tool.shot_recovery, this->shoot_interval()) : tool.shot_recovery;
    motion.pressure = std::max(0.0f, motion.pressure - dt / recovery);
    const bool stream = this->type_ == WeaponType::sprayer || this->type_ == WeaponType::mister;
    if (shot && (!stream || motion.pressure <= 0.0f))
    {
        // The fast streams finish their pressure phrase before a later paid
        // shot starts the next. Holding a zero-interval sprayer cannot pin its
        // hose/valve permanently on the first pose.
        motion.pressure = 1.0f;
    }
    const float valve_target = this->shooting_this_update_ ? 1.0f : 0.0f;
    motion.valve += std::clamp(valve_target - motion.valve,
        -dt / tool.valve_time, dt / tool.valve_time);
    motion.mist_tail_remaining = std::max(0.0f, motion.mist_tail_remaining - dt);
    if (this->type_ == WeaponType::mister)
    {
        const bool flowing = this->shooting_this_update_ && (this->motion_firing_ || shot);
        if (flowing)
        {
            // A fresh pressure stream absorbs any previous release haze.
            motion.mist_tail_remaining = 0.0f;
        }
        else if (this->motion_firing_)
        {
            motion.mist_tail_remaining = look.mist_tail_duration;
            this->mist_release_position_ = this->nozzle_pos();
            this->mist_release_rotation_ = this->rotation();
        }
        this->motion_firing_ = flowing;
    }
    this->select_motion_pose();
}

void Weapon::update_resolved_motion(bool supported, bool established_support,
    float signed_distance, float dt)
{
    if (dt <= 0.0f || this->motion_presentation_ == nullptr ||
        !this->motion_presentation_->enabled) { return; }
    this->advance_travel_motion(supported, established_support, signed_distance, dt);
    this->select_motion_pose();
}

void Weapon::advance_travel_motion(bool supported, bool established_support,
    float signed_distance, float dt)
{
    const auto& look = *this->motion_presentation_;
    auto& motion = this->motion_;
    if (this->type_ == WeaponType::bucket || this->type_ == WeaponType::sprayer)
    {
        // Walking drives the hose, handle and liquid; at rest they settle.
        // No player velocity or aim is changed by this visual accumulator.
        const bool bucket = this->type_ == WeaponType::bucket;
        float& swing = bucket ? motion.bucket_swing : motion.hose_sway;
        const float period = bucket ? look.bucket_travel_per_swing : look.hose_travel_per_sway;
        const float settle = bucket ? look.bucket_settle_time : look.hose_settle_time;
        if (supported && established_support && signed_distance != 0.0f)
        {
            motion.travel_phase = std::fmod(motion.travel_phase +
                signed_distance / period, 1.0f);
            if (motion.travel_phase < 0.0f) { motion.travel_phase += 1.0f; }
            swing = std::sin(motion.travel_phase * PI * 2.0f);
        }
        else
        {
            swing += std::clamp(-swing, -dt / settle, dt / settle);
        }
    }
}

void Weapon::select_motion_pose()
{
    this->motion_.pose = this->motion_pose();
}

ToolMotionPose Weapon::motion_pose() const
{
    const auto& look = *this->motion_presentation_;
    const auto& motion = this->motion_;
    if (motion.pressure > 0.0f)
    {
        return motion.pressure >= 1.0f - look.shot_pose_split ?
            ToolMotionPose::motion_1 : ToolMotionPose::motion_2;
    }
    if (this->type_ == WeaponType::bucket && motion.bucket_swing != 0.0f)
    {
        return motion.bucket_swing > 0.0f ? ToolMotionPose::motion_1 : ToolMotionPose::motion_2;
    }
    if (this->type_ == WeaponType::sprayer && motion.hose_sway != 0.0f)
    {
        return motion.hose_sway > 0.0f ? ToolMotionPose::motion_1 : ToolMotionPose::motion_2;
    }
    if ((this->type_ == WeaponType::sprayer || this->type_ == WeaponType::mister) &&
        motion.valve > 0.0f)
    {
        return ToolMotionPose::motion_2;
    }
    return ToolMotionPose::idle;
}

ToolMotionState& Weapon::tool_motion()
{
    return this->motion_;
}

const ToolMotionState& Weapon::tool_motion() const
{
    return this->motion_;
}

const ToolMotionPresentation& Weapon::motion_look() const
{
    return *this->motion_presentation_;
}

void Weapon::draw_mist_tail(DrawList& draw_list) const
{
    if (this->preferences_->reduce_effects || this->motion_presentation_ == nullptr ||
        !this->motion_presentation_->enabled ||
        this->motion_.mist_tail_remaining <= 0.0f) { return; }
    const auto& look = *this->motion_presentation_;
    const float fade = this->motion_.mist_tail_remaining / look.mist_tail_duration;
    const float progress = 1.0f - fade;
    const Vector2F size = look.mist_tail_size * (1.0f + progress);
    const Vector2F direction = Vector2F::unit_vec_from_angle(this->mist_release_rotation_);
    const Vector2F position = this->mist_release_position_ + direction * (look.mist_tail_distance * progress);
    const auto* sheet = this->render_resources()->sprite_sheet(this->mist_sheet_);
    const RectangleI& source = sheet->sprite_frame(this->mist_frame_).source_rectangle();
    const Vector2F origin(static_cast<float>(source.width) / 2.0f,
        static_cast<float>(source.height) / 2.0f);
    sheet->draw(draw_list, this->mist_frame_, RectangleF(position, size),
        this->paint_override_.value_or(this->paint_colour()) * (fade * look.mist_tail_opacity),
        this->mist_release_rotation_, origin);
}
