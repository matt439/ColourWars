#include "game/pch.h"
#include "game/objects/weapon_roller.h"

using namespace mattmath;
using namespace labrador;


WeaponRoller::WeaponRoller(const GameTuning* tuning,
    PlayerTeam team,
    int player_num,
    int sound_index,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const PlaySettings* preferences,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    Weapon(tuning,
        team, player_num, sound_index, team_colour, type, player_center,
        render_resources, audio_resources, preferences, color, rotation, origin, flip, layer_depth)
{
    const WeaponFeedbackSounds& feedback = this->weapon_tuning_->feedback_sounds;
    if (feedback.lift.has_value())
    {
        this->lift_sound_ = this->resolve_wave(feedback.lift->name);
    }
    if (feedback.settle.has_value())
    {
        this->settle_sound_ = this->resolve_wave(feedback.settle->name);
    }
}

void WeaponRoller::update_ground_contact(bool supported, bool established_support,
    float dt)
{
    if (dt <= 0.0f)
    {
        return;
    }
    if (established_support && this->had_ground_support_ != supported &&
        this->shooting_this_update())
    {
        const WeaponFeedbackSounds& feedback = this->weapon_tuning_->feedback_sounds;
        const std::optional<SoundTuning>& cue = supported ? feedback.settle : feedback.lift;
        if (cue.has_value())
        {
            this->play_feedback_sound(supported ? this->settle_sound_ : this->lift_sound_,
                *cue);
        }
    }
    this->had_ground_support_ = supported;
}

void WeaponRoller::update_movement_and_rotation(PlayerInputData /*input*/,
    const Vector2F& player_center,
    const Vector2F& /*player_velocity*/,
    bool player_facing_right)
{
    if (player_facing_right)  
    {
        this->set_rotation(this->weapon_tuning_->aim_tilt);
    }
    else
    {
        this->set_rotation(PI - this->weapon_tuning_->aim_tilt);
    }
    this->set_player_center(player_center);

    if (facing_left(this->rotation()))
    {
        this->set_invert_y(true);
    }
    else
    {
        this->set_invert_y(false);
    }
}

void WeaponRoller::advance_travel_motion(bool supported, bool established_support,
    float signed_distance, float dt)
{
    const ToolMotionPresentation& look = this->motion_look();
    ToolMotionState& motion = this->tool_motion();
    // Ground contact, rather than input or pre-collision velocity, turns
    // the sleeve. Reversing direction reverses the same accumulated turn.
    if (supported && established_support)
    {
        motion.travel_phase = std::fmod(motion.travel_phase +
            signed_distance / look.roller_travel_per_turn, 1.0f);
        if (motion.travel_phase < 0.0f) { motion.travel_phase += 1.0f; }
    }
    const float lift_target = supported ? 0.0f : 1.0f;
    motion.lift += std::clamp(lift_target - motion.lift,
        -dt / look.roller_lift_time, dt / look.roller_lift_time);
    motion.compression = std::max(0.0f,
        motion.compression - dt / look.roller_settle_time);
    if (supported && !this->motion_supported_ && established_support)
    {
        motion.compression = 1.0f;
    }
    this->motion_supported_ = supported;
}

ToolMotionPose WeaponRoller::motion_pose() const
{
    const ToolMotionState& motion = this->tool_motion();
    if (motion.lift > 0.0f) { return ToolMotionPose::lift; }
    if (motion.compression > 0.0f) { return ToolMotionPose::motion_1; }
    if (motion.travel_phase >= 0.25f && motion.travel_phase < 0.5f)
    {
        return ToolMotionPose::motion_1;
    }
    if (motion.travel_phase >= 0.5f && motion.travel_phase < 0.75f)
    {
        return ToolMotionPose::motion_2;
    }
    return ToolMotionPose::idle;
}

labrador::Colour WeaponRoller::paint_colour() const
{
    return this->shooting_this_update_
        ? this->team_colour()
        : labrador::Colour::white;
}
