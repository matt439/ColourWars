#pragma once

#include "game/carried/sound_bank_object.h"
#include "game/config/tuning.h"
#include "game/objects/player_team.h"
#include "game/objects/projectile.h"
#include "game/objects/sub_type.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/colour.h"
#include "engine/render/render_resources.h"

#include <memory>
#include <vector>

class Weapon;

// A player's sub weapon (ROAD#11, docs/feature-roadmap.md#sub-weapons-road11):
// the second tool beside the chosen one, fired by the Sub action. It has no
// tank of its own; each use spends a share of the tool's (Weapon::spend_paint)
// and then waits out a cooldown. Player owns one and calls update on every
// step it can act. Concrete subs supply what a use releases.
class SubWeapon : public colourwars::SoundBankObject
{
public:
	// Borrowed tuning and resources must outlive the sub. player_num is the
	// paint owner, as for the tool's own shots.
	SubWeapon(const GameTuning* tuning,
		SubType type,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources);
	virtual ~SubWeapon() = default;
	SubWeapon(const SubWeapon&) = delete;
	SubWeapon& operator=(const SubWeapon&) = delete;

	// One step of an acting player. `pressed` is the Sub action's press edge.
	// A press past the cooldown that the tank can pay for releases the sub
	// from `origin` along `direction` (a unit vector), adding the player's
	// horizontal velocity; a press the tank cannot pay for sounds empty.
	// Transfers ownership of non-null projectiles to the caller, which
	// resolves their launch as it does a tool's shot.
	std::vector<std::unique_ptr<Projectile>> update(bool pressed, Weapon& tank,
		const labrador::Vector2F& origin, const labrador::Vector2F& direction,
		const labrador::Vector2F& player_velocity, float dt);

	SubType type() const { return this->type_; }
	// What one use takes from the tank, in tank units.
	float cost() const { return this->sub_tuning_->cost; }
	// Seconds before the next use may leave; zero when it may now.
	float cooldown_remaining() const { return this->cooldown_remaining_; }
	// Whether a press now would release the sub: past its cooldown, with the
	// whole cost in the tank (or Endless paint). What a bot or a HUD reads.
	bool ready(const Weapon& tank) const;
	// Respawn and the match's start: no cooldown carried over.
	void reset();

protected:
	const SubTuning& tuning() const { return *this->sub_tuning_; }
	PlayerTeam team() const { return this->team_; }
	int player_num() const { return this->player_num_; }
	const labrador::Colour& team_colour() const { return this->team_colour_; }
	labrador::RenderResources* render_resources() const { return this->render_resources_; }

	// What one paid use releases, centred at origin with this velocity: the
	// aim's unit direction times throw_speed, plus the player's run.
	// direction is the aim alone, for a sub that is set down rather than
	// thrown (the wall).
	virtual std::vector<std::unique_ptr<Projectile>> release(const labrador::Vector2F& origin,
		const labrador::Vector2F& direction, const labrador::Vector2F& velocity) const = 0;

private:
	const SubTuning* sub_tuning_ = nullptr;
	SubType type_ = SubType::grenade;
	PlayerTeam team_ = PlayerTeam::none;
	int player_num_ = -1;
	labrador::Colour team_colour_ = labrador::Colour::gray;
	labrador::RenderResources* render_resources_ = nullptr;
	labrador::SoundBank::WaveHandle throw_sound_;
	labrador::SoundBank::WaveHandle empty_sound_;
	float cooldown_remaining_ = 0.0f;
};

// The paint grenade: one ProjectileGrenade per use, bursting where it lands.
class SubGrenade final : public SubWeapon
{
public:
	SubGrenade(const GameTuning* tuning,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources);

protected:
	std::vector<std::unique_ptr<Projectile>> release(const labrador::Vector2F& origin,
		const labrador::Vector2F& direction, const labrador::Vector2F& velocity) const override;

private:
	// Resolved once, so a throw resolves no name.
	labrador::RenderResources::SheetStrip body_strip_;
	labrador::RenderResources::SheetStrip burst_strip_;
};

// The sprinkler: one ProjectileSprinkler per use, which sticks where it
// lands and sprays from there.
class SubSprinkler final : public SubWeapon
{
public:
	SubSprinkler(const GameTuning* tuning,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources);

protected:
	std::vector<std::unique_ptr<Projectile>> release(const labrador::Vector2F& origin,
		const labrador::Vector2F& direction, const labrador::Vector2F& velocity) const override;

private:
	labrador::RenderResources::SheetStrip body_strip_;
	labrador::RenderResources::SheetStrip spray_strip_;
};

// The paint wall: one ProjectileWall per use, set down `reach` to the side
// the aim points (right when it points straight up or down), its foot level
// with the player's, and dropped at throw_speed until it stands on cover.
class SubWall final : public SubWeapon
{
public:
	SubWall(const GameTuning* tuning,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources);

protected:
	std::vector<std::unique_ptr<Projectile>> release(const labrador::Vector2F& origin,
		const labrador::Vector2F& direction, const labrador::Vector2F& velocity) const override;

private:
	labrador::RenderResources::SheetStrip body_strip_;
	labrador::RenderResources::SheetStrip spray_strip_;
	// The player's height, so the wall's foot starts level with theirs.
	float player_height_ = 0.0f;
};

// Builds the sub of a type; throws std::invalid_argument for any other value.
std::unique_ptr<SubWeapon> build_sub_weapon(const GameTuning* tuning,
	SubType type,
	PlayerTeam team,
	int player_num,
	const labrador::Colour& team_colour,
	labrador::RenderResources* render_resources,
	const labrador::AudioResources* audio_resources);
