#include "game/pch.h"
#include "game/objects/weapon.h"

#include <limits>

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

namespace
{
    // Debug nozzle marker size and sprite frame.
    const Vector2F nozzle_marker_size = { 5.0f, 5.0f };
    const std::string nozzle_frame_name = "nozzle";
}

Weapon::Weapon(const GameTuning* tuning,
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
    TextureObject(tuning->weapon(type).details.sheet_name,
                  tuning->weapon(type).details.frame_name, render_resources,
                  color, rotation, origin, flip, layer_depth),
        tuning_(tuning),
        weapon_tuning_(&tuning->weapon(type)),
        render_resources_(render_resources),
        team_(team),
        player_num_(player_num),
        team_colour_(team_colour),
        type_(type),
        player_center_(player_center)
{
    this->preferences_ = preferences;
    const WeaponDetails& details = this->details();

    this->ammo_ = tuning->starting_ammo();
    this->proj_builder_ = std::make_unique<ProjectileBuilder>(tuning);
    this->sound_bank_ = audio_resources->sound_bank(details.sound_bank_name);
    this->loop_sound_ = resolve_loop_sound(*this->sound_bank_,
        this->weapon_tuning_->loop_sounds, team, sound_index);
    this->shoot_sound_ = this->sound_bank_->resolve_wave(
        details.shoot_sound_name);
    const WeaponFeedbackSounds& feedback = this->weapon_tuning_->feedback_sounds;
    this->empty_sound_ = this->sound_bank_->resolve_wave(feedback.empty.name);
    this->recovered_sound_ = this->sound_bank_->resolve_wave(feedback.recovered.name);
    if (feedback.stop.has_value())
    {
        this->stop_sound_ = this->sound_bank_->resolve_wave(feedback.stop->name);
    }
    if (feedback.ready.has_value())
    {
        this->ready_sound_ = this->sound_bank_->resolve_wave(feedback.ready->name);
    }
    this->paint_frame_ = this->sprite_sheet()->
        resolve_sprite_frame(details.frame_name + "_paint");
    this->nozzle_frame_ = this->sprite_sheet()->
        resolve_sprite_frame(nozzle_frame_name);
}

void Weapon::draw(DrawList& draw_list, bool debug) const
{
    Vector2F draw_pos = this->draw_pos();
    auto draw_rectangle = RectangleF(draw_pos, this->details().size);

    // Sprite origins are source texels, independent of the gameplay size.
    // Keep the grip centred when an art pass changes the texture resolution.
    const RectangleI& source = this->sprite_sheet()->sprite_frame(
        this->frame()).source_rectangle();
    const Vector2F origin(0.0f, static_cast<float>(source.height) / 2.0f);

    // Left-facing weapons combine rotation past +/-PI/2 with a vertical flip.
    SpriteFlip flip = this->invert_y()
        ? SpriteFlip::vertical
        : SpriteFlip::none;

    // Keep draw state local because render workers share this weapon.
    const bool motion_enabled = this->motion_presentation_ != nullptr &&
        this->motion_presentation_->enabled;
    const size_t pose = static_cast<size_t>(this->motion_.pose);
    TextureObject::draw_with(draw_list, draw_rectangle,
        motion_enabled ? this->motion_frames_[pose] : this->frame(), this->colour(),
        origin, flip, this->rotation());
    TextureObject::draw_with(draw_list, draw_rectangle,
        motion_enabled ? this->motion_paint_frames_[pose] : this->paint_frame_,
        this->paint_override_.value_or(this->paint_colour()),
        origin, flip, this->rotation());

    this->draw_mist_tail(draw_list);
    if (this->feedback_ != nullptr)
    {
        this->feedback_->draw(draw_list, this->nozzle_pos(), this->draw_pos(),
            this->rotation(), this->type(), this->paint_override_.value_or(this->team_colour()));
    }

    if (debug)
    {
        this->draw_nozzle_debug(draw_list);
    }
}

void Weapon::draw_nozzle_debug(DrawList& draw_list) const
{
    const RectangleI& source = this->sprite_sheet()->sprite_frame(
        this->nozzle_frame_).source_rectangle();
    const Vector2F origin(static_cast<float>(source.width) / 2.0f,
        static_cast<float>(source.height) / 2.0f);
    TextureObject::draw_with(draw_list, this->nozzle_rectangle(),
        this->nozzle_frame_, this->colour(), origin, SpriteFlip::none, 0.0f);
}

void Weapon::set_paint_override(std::optional<Colour> colour)
{
    this->paint_override_ = colour;
}

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

void Weapon::set_feedback_presentation(const WeaponFeedbackPresentation* presentation)
{
    this->feedback_ = presentation == nullptr ? nullptr :
        std::make_unique<WeaponFeedback>(presentation, this->render_resources());
}

const WeaponFeedback* Weapon::feedback() const { return this->feedback_.get(); }

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
    const auto& look = *this->motion_presentation_;
    auto& motion = this->motion_;
    if (this->type_ == WeaponType::roller)
    {
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
    else if (this->type_ == WeaponType::bucket || this->type_ == WeaponType::sprayer)
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
    this->select_motion_pose();
}

void Weapon::select_motion_pose()
{
    const auto& look = *this->motion_presentation_;
    auto& motion = this->motion_;
    motion.pose = ToolMotionPose::idle;
    if (this->type_ == WeaponType::roller)
    {
        if (motion.lift > 0.0f) { motion.pose = ToolMotionPose::lift; }
        else if (motion.compression > 0.0f) { motion.pose = ToolMotionPose::motion_1; }
        else if (motion.travel_phase >= 0.25f && motion.travel_phase < 0.5f)
        {
            motion.pose = ToolMotionPose::motion_1;
        }
        else if (motion.travel_phase >= 0.5f && motion.travel_phase < 0.75f)
        {
            motion.pose = ToolMotionPose::motion_2;
        }
        return;
    }
    if (motion.pressure > 0.0f)
    {
        motion.pose = motion.pressure >= 1.0f - look.shot_pose_split ?
            ToolMotionPose::motion_1 : ToolMotionPose::motion_2;
    }
    else if (this->type_ == WeaponType::bucket && motion.bucket_swing != 0.0f)
    {
        motion.pose = motion.bucket_swing > 0.0f ? ToolMotionPose::motion_1 : ToolMotionPose::motion_2;
    }
    else if (this->type_ == WeaponType::sprayer && motion.hose_sway != 0.0f)
    {
        motion.pose = motion.hose_sway > 0.0f ? ToolMotionPose::motion_1 : ToolMotionPose::motion_2;
    }
    else if ((this->type_ == WeaponType::sprayer || this->type_ == WeaponType::mister) &&
        motion.valve > 0.0f)
    {
        motion.pose = ToolMotionPose::motion_2;
    }
}

void Weapon::draw_mist_tail(DrawList& draw_list) const
{
    if (this->motion_presentation_ == nullptr || !this->motion_presentation_->enabled ||
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

labrador::Colour Weapon::paint_colour() const
{
    return this->team_colour();
}

bool Weapon::shooting_this_update() const
{
    return this->shooting_this_update_;
}
Vector2F Weapon::calculate_sprite_origin(const Vector2F& size, RotationOrigin origin)
{
    switch (origin)
    {
    case RotationOrigin::center:
        return Vector2F(size) / 2.0f;
    case RotationOrigin::left_center:
        return {0.0f, size.y / 2.0f};
    case RotationOrigin::top_left:
        return Vector2F::ZERO;
    default:
        return Vector2F::ZERO;
    }
}

RectangleF Weapon::nozzle_rectangle() const
{
	Vector2F nozzle_pos = this->nozzle_pos();

    return { nozzle_pos, nozzle_marker_size };
}

Vector2F Weapon::nozzle_size()
{
	return nozzle_marker_size;
}




RectangleF Weapon::draw_bounds() const
{
	const Vector2F size = this->details().size;
	const Vector2F centre = this->draw_pos();

	// One sprite diagonal covers any rotation about an origin inside the sprite.
	const float reach = size.length();
	RectangleF result(centre - Vector2F(reach, reach), Vector2F(reach * 2.0f, reach * 2.0f));
	if (this->motion_presentation_ != nullptr && this->motion_presentation_->enabled &&
		this->motion_.mist_tail_remaining > 0.0f)
	{
		// The released haze stays where it was emitted when the player moves.
		// Its maximum doubled size has a half diagonal equal to the base one.
		const float tail_reach = this->motion_presentation_->mist_tail_distance +
			this->motion_presentation_->mist_tail_size.length();
		result = RectangleF::union_of(result, RectangleF(
			this->mist_release_position_ - Vector2F(tail_reach, tail_reach),
			Vector2F(tail_reach * 2.0f, tail_reach * 2.0f)));
	}
	if (this->feedback_ != nullptr)
	{
		result = RectangleF::union_of(result,
			this->feedback_->bounds(this->nozzle_pos(), this->draw_pos(), this->type()));
	}
	return result;
}
Vector2F Weapon::draw_pos() const
{
	Vector2F result = this->player_center();
	if (this->facing_left(this->rotation_))
	{
		result.x -= this->details().offset.x;
		result.y += this->details().offset.y;
	}
	else
	{
		result += this->details().offset;
	}
	return result;
}

bool Weapon::facing_left(float rotation)
{
	if (rotation > PI / 2.0f ||
		rotation < PI / -2.0f)
	{
		return true;
	}
	return false;
}

Vector2F Weapon::nozzle_pos() const
{
	Vector2F result = this->details().nozzle_offset;
	result.x += this->details().size.x;
	result = Vector2F::rotate_vector(result, this->rotation_);
	result += this->wep_rotation_origin_offset(
		facing_left(this->rotation_));
	result += this->player_center();
	return result;
}

Vector2F Weapon::wep_rotation_origin_offset(bool facing_left) const
{
	if (facing_left)
	{
		return Vector2F(-this->details().offset.x,
			this->details().offset.y);
	}
	return Vector2F(this->details().offset.x, this->details().offset.y);
}

std::vector<std::unique_ptr<Projectile>>
    Weapon::update_and_get_projectiles(PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& player_velocity,
    bool player_facing_right,
    float dt)
{
    this->update_movement_and_rotation(input, player_center,
		player_velocity, player_facing_right);

    if (this->feedback_ != nullptr) { this->feedback_->update(dt); }

    const bool shot = this->check_if_shooting_and_ammo_update(input, player_center,
        player_velocity, dt);
    this->update_motion(shot, dt);
    if (shot && dt > 0.0f && std::isfinite(dt) && this->feedback_ != nullptr)
    {
        this->feedback_->shot(this->type());
    }
    if (shot)
    {
        return this->shoot(Vector2F::unit_vec_from_angle(this->rotation()),
            player_velocity);
	}
	else
	{
		return {};
    }
}

bool Weapon::check_if_shooting_and_ammo_update(
    PlayerInputData input,
    const Vector2F& /*player_center*/,
    const Vector2F& /*player_velocity*/,
    float dt)
{
    const bool ok_to_shoot = input.primary_shoot;
    const bool affordable_before = this->can_afford_shot();
    bool result = false;

    if (this->ready_to_shoot() && ok_to_shoot)
    {
        result = true;

        this->set_shoot_timer(0.0f);
        this->alter_ammo(-this->ammo_usage());
        if (this->ammo() < 0.0f)
        {
            this->set_ammo(0.0f);
        }
        this->set_ammo_timer(0.0f);
        if (dt > 0.0f)
        {
            this->awaiting_recovery_ = this->awaiting_recovery_ ||
                !this->can_afford_shot();
            this->cooldown_feedback_pending_ = true;
        }
    }

    // Regenerate up to the configured starting ammo.
    if (this->ammo_timer() >= this->tuning_->ammo_regen_delay())
    {
        const float max_ammo = this->tuning_->starting_ammo();
        this->alter_ammo(this->tuning_->ammo_regen_rate() * dt);
        if (this->ammo() > max_ammo)
        {
            this->set_ammo(max_ammo);
        }
    }
    this->alter_ammo_timer(dt);
    this->alter_shoot_timer(dt);

    // Keep the final paid shot visible/audible for its tick, then stop the
    // held-fire presentation while waiting for enough ammo for another shot.
    this->shooting_this_update_ = result ||
        (ok_to_shoot && this->can_afford_shot());

    const bool loop_was_playing = this->loop_playing_;
    this->handle_shoot_sound(result, this->shooting_this_update_);
    if (dt > 0.0f && loop_was_playing && !this->loop_playing_ &&
        this->weapon_tuning_->feedback_sounds.stop.has_value())
    {
        // Normal release or depletion only. Silent lifecycle stops bypass this
        // path, so death, time-up, teardown and suspension have no release tail.
        this->play_feedback_sound(this->stop_sound_,
            *this->weapon_tuning_->feedback_sounds.stop);
    }
    this->update_feedback_sounds(ok_to_shoot, result, affordable_before, dt);

    return result;

}
bool Weapon::ready_to_shoot() const
{
    return this->shoot_timer() > this->shoot_interval() && this->can_afford_shot();
}
void Weapon::update_feedback_sounds(bool holding_shoot, bool shot,
    bool affordable_before, float dt)
{
    // Acceptance probes can pay a primed shot at zero delta. Keep that gameplay
    // behaviour, but frozen updates cannot advance or emit the new feedback.
    if (dt <= 0.0f || !std::isfinite(dt))
    {
        return;
    }

    const WeaponFeedbackSounds& feedback = this->weapon_tuning_->feedback_sounds;
    if (!holding_shoot)
    {
        this->empty_warned_ = false;
        this->empty_repeat_remaining_ = 0.0f;
    }
    else
    {
        this->empty_repeat_remaining_ = std::max(0.0f,
            this->empty_repeat_remaining_ - dt);
        if (!shot && !affordable_before &&
            (!this->empty_warned_ || this->empty_repeat_remaining_ <= 0.0f))
        {
            this->play_feedback_sound(this->empty_sound_, feedback.empty);
            if (this->feedback_ != nullptr) { this->feedback_->empty(); }
            this->empty_warned_ = true;
            // Retain the limit across brief recoveries while fire stays held.
            // A long update emits at most one warning, never a queued burst.
            this->empty_repeat_remaining_ = feedback.empty_repeat_delay;
        }
    }

    if (this->awaiting_recovery_ && this->can_afford_shot())
    {
        this->play_feedback_sound(this->recovered_sound_, feedback.recovered);
        if (this->feedback_ != nullptr) { this->feedback_->recovered(); }
        this->awaiting_recovery_ = false;
    }

    if (this->cooldown_feedback_pending_ &&
        this->shoot_timer() > this->shoot_interval())
    {
        // Only a paid shot arms this edge. If paint is still unavailable when
        // its cooldown completes, recovery has its own cue instead.
        this->cooldown_feedback_pending_ = false;
        if (this->ready_to_shoot() && feedback.ready.has_value())
        {
            this->play_feedback_sound(this->ready_sound_, *feedback.ready);
        }
    }
}
void Weapon::play_feedback_sound(SoundBank::WaveHandle sound,
    const SoundTuning& tuning) const
{
    this->sound_bank_->play_wave(sound, tuning.volume * this->preferences_->sfx_gain());
}
void Weapon::update_ground_contact(bool /*supported*/, bool /*established_support*/,
    float /*dt*/)
{
}
bool Weapon::can_afford_shot() const
{
    const float cost = this->ammo_usage();
    // Bound the allowance to less than one millionth of a shot, even when
    // authors choose a tiny cost. A tank-sized epsilon could buy those shots.
    const float rounding = cost * (8.0f * std::numeric_limits<float>::epsilon());
    return this->ammo() >= cost ||
        (this->ammo() > 0.0f && cost - this->ammo() <= rounding);
}
void Weapon::handle_shoot_sound(bool shooting_this_update, bool holding_shoot)
{
    if (!this->loop_sound_.valid())
    {
        return;
    }

    if (holding_shoot || shooting_this_update)
    {
        if (!this->loop_playing_)
        {
            this->sound_bank_->play_wave(this->shoot_sound_,
                this->details().shoot_sound_volume * this->preferences_->sfx_gain());
            this->sound_bank_->play_effect(
                this->loop_sound_, true,
                this->details().shoot_sound_volume * this->preferences_->sfx_gain());
            this->loop_playing_ = true;
        }
    }
    else if (this->loop_playing_)
    {
        this->stop_sounds();
    }
}
void Weapon::stop_sounds() const
{
    if (!this->loop_sound_.valid())
    {
        return;
    }
    this->sound_bank_->stop_effect(this->loop_sound_, true);
    this->loop_playing_ = false;
}
void Weapon::pause_sounds() const
{
    if (!this->loop_sound_.valid())
    {
        return;
    }
    this->sound_bank_->pause_effect(this->loop_sound_);
}
void Weapon::resume_sounds() const
{
    if (!this->loop_sound_.valid())
    {
        return;
    }
    this->sound_bank_->set_effect_volume(this->loop_sound_,
        this->details().shoot_sound_volume * this->preferences_->sfx_gain());
    this->sound_bank_->resume_effect(this->loop_sound_);
}
SoundBank::EffectHandle Weapon::resolve_loop_sound(const SoundBank& sound_bank,
    const WeaponLoopSounds& loop_sounds, PlayerTeam team, int sound_index)
{
    // One-shot weapons have no loop_sounds record and use an invalid handle.
    // Resolve loops at construction to avoid name lookups during firing or teardown.
    if (team != PlayerTeam::a && team != PlayerTeam::b)
    {
        return {};
    }
    if (sound_index < 0 || sound_index > 3)
    {
        return {};
    }

    if (!loop_sounds.present)
    {
        return {};
    }

    return sound_bank.resolve_effect(loop_sounds.sound_name(team, sound_index));
}
void Weapon::update_movement_and_rotation(PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& /*player_velocity*/,
    bool player_facing_right)
{
    if (input.shoot_direction_requested)
    {
        this->set_rotation(input.shoot_angle);
        this->aim_requested_ = true;
    }
    else if (!this->aim_requested_)
    {
		if (player_facing_right)
		{
			this->set_rotation(0.0f);
		}
		else
		{
			this->set_rotation(PI);
		}
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

std::vector<std::unique_ptr<Projectile>>
Weapon::shoot(const Vector2F& shoot_direction,
    const Vector2F& player_velocity) const
{
    Vector2F launch_velocity = this->calculate_projectile_launch_velocity(
        shoot_direction, this->starting_vel_length(), player_velocity);

    return this->proj_builder_->build_projectiles(
        this->nozzle_pos(),
        launch_velocity,
        this->team(),
        this->player_num(),
        this->team_colour(),
        this->details().proj_type,
        this->render_resources());
}

Vector2F Weapon::calculate_projectile_launch_velocity(
    const Vector2F& shoot_direction,
    float starting_velocity,
    const Vector2F& /*player_velocity*/) const
{
    Vector2F result = shoot_direction * starting_velocity;
    return result;
}

void Weapon::set_shoot_timer(float shoot_timer)
{
	this->shoot_timer_ = shoot_timer;
}

void Weapon::alter_shoot_timer(float dt)
{
	this->shoot_timer_ += dt;
}

float Weapon::shoot_timer() const
{
	return this->shoot_timer_;
}

void Weapon::set_ammo(float ammo)
{
	this->ammo_ = ammo;
}

void Weapon::alter_ammo(float ammo)
{
	this->ammo_ += ammo;
}

float Weapon::ammo_timer() const
{
	return this->ammo_timer_;
}

void Weapon::set_ammo_timer(float ammo_timer)
{
	this->ammo_timer_ = ammo_timer;
}

void Weapon::alter_ammo_timer(float dt)
{
	this->ammo_timer_ += dt;
}

float Weapon::ammo() const
{
	return this->ammo_;
}

const WeaponDetails& Weapon::details() const
{
	return this->weapon_tuning_->details;
}

const Vector2F& Weapon::offset() const
{
	return this->details().offset;
}

const Vector2F& Weapon::nozzle_offset() const
{
	return this->details().nozzle_offset;
}

float Weapon::shoot_interval() const
{
	return this->details().shoot_interval;
}

float Weapon::starting_vel_length() const
{
	return this->details().starting_vel_length;
}

float Weapon::ammo_usage() const
{
	return this->details().ammo_usage;
}

float Weapon::rotation() const
{
	return this->rotation_;
}

void Weapon::set_rotation(float rotation)
{
	this->rotation_ = rotation;
}

bool Weapon::invert_y() const
{
	return this->invert_y_;
}

void Weapon::set_invert_y(bool invert_y)
{
	this->invert_y_ = invert_y;
}

const Vector2F& Weapon::player_center() const
{
	return this->player_center_;
}

void Weapon::set_player_center(const Vector2F& player_center)
{
	this->player_center_ = player_center;
}

PlayerTeam Weapon::team() const
{
	return this->team_;
}

int Weapon::player_num() const
{
	return this->player_num_;
}

const Colour& Weapon::team_colour() const
{
	return this->team_colour_;
}

WeaponType Weapon::type() const
{
	return this->type_;
}

void Weapon::reset_ammo(bool reset_feedback)
{
	// Respawn restores the configured full tank.
	this->ammo_ = this->tuning_->starting_ammo();
	// A forced refill is not gameplay recovery, including an infinite-ammo
	// refill immediately after a paid shot temporarily emptied the tank.
	this->awaiting_recovery_ = false;
	if (reset_feedback)
	{
		this->reset_motion();
		this->empty_warned_ = false;
		this->empty_repeat_remaining_ = 0.0f;
		this->cooldown_feedback_pending_ = false;
	}
}

ProjectileBuilder* Weapon::projectile_builder() const
{
	return this->proj_builder_.get();
}
RenderResources* Weapon::render_resources() const
{
	return this->render_resources_;
}

RelativeVelocityWeapon::RelativeVelocityWeapon(
    const GameTuning* tuning,
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
    labrador::SpriteFlip flip,
    float layer_depth) :
    Weapon(tuning, team, player_num, sound_index, team_colour, type,
        player_center, render_resources, audio_resources, preferences,
        color, rotation, origin, flip, layer_depth)
{

}

const RelativeWeaponDetails& RelativeVelocityWeapon::rel_details() const
{
    return this->weapon_tuning_->relative;
}

Vector2F RelativeVelocityWeapon::calculate_projectile_launch_velocity(
    const Vector2F& shoot_direction,
    float starting_velocity,
    const Vector2F& player_velocity) const
{
    const RelativeWeaponDetails& relative = this->rel_details();
    Vector2F result = shoot_direction * starting_velocity;

    // Handle every velocity mode explicitly, including none.
    switch (relative.add_vel)
    {
    case AddPlayerVelocity::x_and_y:
        result += player_velocity * relative.player_vel_amount;
        break;
    case AddPlayerVelocity::x_only:
        result.x += player_velocity.x * relative.player_vel_amount;
        break;
    case AddPlayerVelocity::y_only:
        result.y += player_velocity.y * relative.player_vel_amount;
        break;
    case AddPlayerVelocity::none:
        break;
    }
    return result;
}
