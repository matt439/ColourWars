#include "game/pch.h"
#include "game/objects/weapon.h"

#include <limits>

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
    this->proj_builder_ = std::make_unique<ProjectileBuilder>(tuning, render_resources);
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
    if (feedback.low.has_value())
    {
        this->low_sound_ = this->sound_bank_->resolve_wave(feedback.low->name);
    }
    this->paint_frame_ = this->sprite_sheet()->
        resolve_sprite_frame(details.frame_name + "_paint");
    if (type == WeaponType::sprayer || type == WeaponType::mister)
    {
        this->supply_strip_ = this->sprite_sheet()->resolve_animation_strip(
            details.frame_name + "_supply");
    }
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
    if (this->supply_strip_)
    {
        const AnimationStrip& supply = this->sprite_sheet()->animation_strip(*this->supply_strip_);
        const float remaining = std::clamp(this->ammo() / this->tuning_->starting_ammo(), 0.0f, 1.0f);
        const int level = static_cast<int>(std::ceil(remaining * (supply.frame_count() - 1)));
        // Keep all three layers on the identical full-canvas transform. A
        // cropped destination would snap its edges independently at 720p,
        // moving the liquid off its window during rotation or camera motion.
        this->sprite_sheet()->draw(draw_list, supply.frame_rect(level), draw_rectangle,
            this->paint_override_.value_or(this->paint_colour()), this->rotation(), origin, flip);
    }

    this->draw_mist_tail(draw_list);
    if (this->feedback_ != nullptr)
    {
        this->feedback_->draw(draw_list, this->nozzle_pos(), this->draw_pos(),
            this->rotation(), this->type(), this->paint_override_.value_or(this->team_colour()),
            this->preferences_->reduce_effects,
            this->low_paint_cue());
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

void Weapon::set_feedback_presentation(const WeaponFeedbackPresentation* presentation)
{
    this->feedback_ = presentation == nullptr ? nullptr :
        std::make_unique<WeaponFeedback>(presentation, this->render_resources());
}

const WeaponFeedback* Weapon::feedback() const { return this->feedback_.get(); }

labrador::Colour Weapon::paint_colour() const
{
    return this->team_colour();
}

bool Weapon::shooting_this_update() const
{
    return this->shooting_this_update_;
}
Vector2F Weapon::calculate_sprite_origin(const Vector2F& size, colourwars::RotationOrigin origin)
{
    switch (origin)
    {
    case colourwars::RotationOrigin::center:
        return Vector2F(size) / 2.0f;
    case colourwars::RotationOrigin::left_center:
        return {0.0f, size.y / 2.0f};
    case colourwars::RotationOrigin::top_left:
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
			this->feedback_->bounds(this->nozzle_pos(), this->draw_pos(), this->type(),
				this->low_paint_cue()));
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
    float dt, float ammo_regen_multiplier)
{
    if (!std::isfinite(ammo_regen_multiplier) || ammo_regen_multiplier < 0.0f ||
        (ammo_regen_multiplier != 1.0f && !std::isfinite(this->tuning_->ammo_regen_rate() * ammo_regen_multiplier)))
    { throw std::invalid_argument("Ammunition recovery multiplier must produce a finite nonnegative rate"); }
    this->update_movement_and_rotation(input, player_center,
		player_velocity, player_facing_right);

    if (this->feedback_ != nullptr) { this->feedback_->update(dt); }

    const bool shot = this->check_if_shooting_and_ammo_update(input, player_center,
        player_velocity, dt, ammo_regen_multiplier);
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
    float dt, float ammo_regen_multiplier)
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
        if (ammo_regen_multiplier == 1.0f)
        {
            // Preserve the original expression/order for neutral and direct callers.
            this->alter_ammo(this->tuning_->ammo_regen_rate() * dt);
        }
        else if (ammo_regen_multiplier > 0.0f)
        {
            this->alter_ammo((this->tuning_->ammo_regen_rate() * ammo_regen_multiplier) * dt);
        }
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
    return this->readiness() == WeaponReadiness::ready;
}

WeaponReadiness Weapon::readiness() const
{
    // An empty tool cannot fire even after its cooldown. Reuse the full-shot
    // rounding allowance rather than approximating eligibility from the meter.
    if (!this->can_afford_shot()) { return WeaponReadiness::empty; }
    return this->shoot_timer() > this->shoot_interval()
        ? WeaponReadiness::ready : WeaponReadiness::recovering;
}

void Weapon::draw_aim_marker(DrawList& draw_list) const
{
    if (this->type() == WeaponType::roller || this->feedback_ == nullptr) { return; }
    this->feedback_->draw_aim_marker(draw_list, this->nozzle_pos(), this->rotation(),
        this->readiness(), this->team_colour());
}

RectangleF Weapon::aim_marker_bounds() const
{
    if (this->type() == WeaponType::roller || this->feedback_ == nullptr)
    {
        return RectangleF(this->nozzle_pos(), Vector2F::ZERO);
    }
    return this->feedback_->aim_marker_bounds(this->nozzle_pos(), this->rotation());
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
            this->pending_rumble_.dry_fire = true;
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

    // One warning as a paid shot leaves the tool low. Held fire cannot repeat
    // it; only recovery past the rearm line does, so it never chatters.
    if (this->low_warned_ && this->ammo() >= this->low_paint_rearm_level())
    {
        this->low_warned_ = false;
    }
    if (shot && !this->low_warned_ && this->low_paint())
    {
        if (feedback.low.has_value())
        {
            this->play_feedback_sound(this->low_sound_, *feedback.low);
        }
        this->low_warned_ = true;
    }
    this->low_elapsed_ = this->low_paint() ? this->low_elapsed_ + dt : 0.0f;

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
    this->sound_bank_->play_wave(sound, tuning.volume);
}
void Weapon::update_ground_contact(bool /*supported*/, bool /*established_support*/,
    float /*dt*/)
{
}
float Weapon::low_paint_level() const
{
    const LowPaintTuning& low = this->tuning_->low_paint();
    const float full = this->tuning_->starting_ammo();
    return std::min(full, std::max(low.supply_fraction * full, low.shots * this->ammo_usage()));
}
float Weapon::low_paint_rearm_level() const
{
    const float full = this->tuning_->starting_ammo();
    return std::min(full, this->low_paint_level() +
        std::max(this->tuning_->low_paint().rearm_fraction * full, this->ammo_usage()));
}
bool Weapon::low_paint() const { return this->ammo() < this->low_paint_level(); }
float Weapon::low_paint_elapsed() const { return this->low_elapsed_; }
bool Weapon::low_paint_warned() const { return this->low_warned_; }
LowPaintCue Weapon::low_paint_cue() const
{
    if (!this->low_paint()) { return {}; }
    return { true, this->ammo() / this->low_paint_level(), this->low_elapsed_,
        this->player_center().x };
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
                this->details().shoot_sound_volume);
            this->sound_bank_->play_effect(
                this->loop_sound_, true,
                this->details().shoot_sound_volume);
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
	this->low_warned_ = false;
	this->low_elapsed_ = 0.0f;
	if (reset_feedback)
	{
		this->clear_rumble_events();
		this->reset_motion();
		this->empty_warned_ = false;
		this->empty_repeat_remaining_ = 0.0f;
		this->cooldown_feedback_pending_ = false;
	}
}

RumbleEvents Weapon::take_rumble_events() noexcept
{
	const RumbleEvents result = this->pending_rumble_;
	this->pending_rumble_ = {};
	return result;
}

void Weapon::clear_rumble_events() noexcept
{
	this->pending_rumble_ = {};
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
