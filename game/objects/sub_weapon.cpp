#include "game/pch.h"
#include "game/objects/sub_weapon.h"
#include "game/objects/projectile_grenade.h"
#include "game/objects/projectile_sprinkler.h"
#include "game/objects/weapon.h"

#include <cmath>
#include <stdexcept>

using namespace labrador;

SubWeapon::SubWeapon(const GameTuning* tuning,
	SubType type,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	const AudioResources* audio_resources) :
	colourwars::SoundBankObject(tuning->sub(type).sound_bank, audio_resources),
	sub_tuning_(&tuning->sub(type)),
	type_(type),
	team_(team),
	player_num_(player_num),
	team_colour_(team_colour),
	render_resources_(render_resources)
{
	this->throw_sound_ = this->resolve_wave(this->sub_tuning_->throw_sound.name);
	this->empty_sound_ = this->resolve_wave(this->sub_tuning_->empty_sound.name);
}

std::vector<std::unique_ptr<Projectile>> SubWeapon::update(bool pressed, Weapon& tank,
	const Vector2F& origin, const Vector2F& direction,
	const Vector2F& player_velocity, float dt)
{
	if (!(dt > 0.0f) || !std::isfinite(dt))
	{
		return {};
	}
	this->cooldown_remaining_ = std::max(0.0f, this->cooldown_remaining_ - dt);
	if (!pressed || this->cooldown_remaining_ > 0.0f)
	{
		return {};
	}
	const SubTuning& tuning = this->tuning();
	if (!tank.spend_paint(tuning.cost))
	{
		this->play_wave(this->empty_sound_, tuning.empty_sound.volume);
		return {};
	}
	this->cooldown_remaining_ = tuning.cooldown;
	this->play_wave(this->throw_sound_, tuning.throw_sound.volume);
	// The bucket's rule: a throw carries the thrower's run, not their fall.
	const Vector2F velocity = direction * tuning.throw_speed +
		Vector2F(player_velocity.x, 0.0f);
	return this->release(origin, velocity);
}

bool SubWeapon::ready(const Weapon& tank) const
{
	return this->cooldown_remaining_ <= 0.0f && tank.can_pay(this->tuning().cost);
}

void SubWeapon::reset()
{
	this->cooldown_remaining_ = 0.0f;
}

SubGrenade::SubGrenade(const GameTuning* tuning,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	const AudioResources* audio_resources) :
	SubWeapon(tuning, SubType::grenade, team, player_num, team_colour,
		render_resources, audio_resources)
{
	this->body_strip_ = resolve_projectile_strip(this->tuning().body, *render_resources);
	this->burst_strip_ = resolve_projectile_strip(this->tuning().burst, *render_resources);
}

std::vector<std::unique_ptr<Projectile>> SubGrenade::release(
	const Vector2F& origin, const Vector2F& velocity) const
{
	const SubTuning& tuning = this->tuning();
	std::vector<std::unique_ptr<Projectile>> released;
	released.push_back(std::make_unique<ProjectileGrenade>(
		RectangleF(origin, tuning.body.details.col_rect_size), velocity,
		this->team(), this->player_num(), this->team_colour(),
		this->render_resources(), this->body_strip_, &tuning.body,
		this->burst_strip_, &tuning.burst, tuning.burst_count, tuning.burst_speed));
	return released;
}

SubSprinkler::SubSprinkler(const GameTuning* tuning,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	const AudioResources* audio_resources) :
	SubWeapon(tuning, SubType::sprinkler, team, player_num, team_colour,
		render_resources, audio_resources)
{
	this->body_strip_ = resolve_projectile_strip(this->tuning().body, *render_resources);
	this->spray_strip_ = resolve_projectile_strip(this->tuning().burst, *render_resources);
}

std::vector<std::unique_ptr<Projectile>> SubSprinkler::release(
	const Vector2F& origin, const Vector2F& velocity) const
{
	const SubTuning& tuning = this->tuning();
	std::vector<std::unique_ptr<Projectile>> released;
	released.push_back(std::make_unique<ProjectileSprinkler>(
		RectangleF(origin, tuning.body.details.col_rect_size), velocity,
		this->team(), this->player_num(), this->team_colour(),
		this->render_resources(), this->body_strip_, this->spray_strip_, &tuning));
	return released;
}

std::unique_ptr<SubWeapon> build_sub_weapon(const GameTuning* tuning,
	SubType type,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	const AudioResources* audio_resources)
{
	switch (type)
	{
	case SubType::grenade:
		return std::make_unique<SubGrenade>(tuning, team, player_num, team_colour,
			render_resources, audio_resources);
	case SubType::sprinkler:
		return std::make_unique<SubSprinkler>(tuning, team, player_num, team_colour,
			render_resources, audio_resources);
	case SubType::count:
		break;
	}
	throw std::invalid_argument("Sub type " + std::to_string(static_cast<int>(type)) +
		" has no builder.");
}
