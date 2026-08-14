#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_details.h"
#include "game/objects/weapon_details.h"
#include "game/objects/weapon_type.h"
#include "engine/math/matt_math.h"
#include "engine/render/colour.h"

#include <array>
#include <optional>
#include <string>

// How much, for every number a paint match is balanced on.
//
// This is T7 and PHILOSOPHY's "tuning changes rebuild nothing". These numbers
// were C++ aggregates in three headers - weapon_consts.h, projectile_consts.h
// and player_consts.h - included by most of game/objects/, so moving a
// projectile's gravity by 0.1 rebuilt a third of the game and shipped as a new
// binary. They are ./tuning.json now: the game reads it at startup like any
// other content, and a balance pass costs a restart.
//
// WHAT IS NOT HERE, AND WHY. C++ says *how* and data says *which* and *how
// much* (PHILOSOPHY, Content and assets). So the shape of a jump - launch,
// then acceleration while held, capped by a time - is C++, and the three
// numbers it takes are here. What is deliberately not here is anything whose
// value changes the *code*: the member defaults a Player is constructed with
// before a builder overwrites them, the debug nozzle marker, and the animation
// *states*, which are an enum the code switches on. A file that could add a
// sixth animation state would be a scripting layer through the back door (T10).

// A named sound and how loud it is played.
struct SoundTuning
{
	std::string name;
	float volume = 1.0f;
};

// How long each phase of a match lasts, and what the countdown looks like.
//
// These were `level_consts` in game/objects/level.h - a SCREAMING_SNAKE
// namespace collecting strays, which is the shape CONVENTIONS names as
// "usually tuning data in the wrong place". The four durations are exactly
// that. The countdown's font, colour and shadow are here for the same reason
// the weapons' sprite frames are: a number or a name a person picks by
// looking at the screen is data (T7).
struct MatchTuning
{
	float duration = 0.0f;
	float countdown = 0.0f;
	float zoom_out = 0.0f;
	float overview = 0.0f;

	std::string countdown_font;
	artattack::Colour countdown_colour = artattack::Colour::white;
	artattack::Colour countdown_shadow_colour = artattack::Colour::black;
	mattmath::Vector2F countdown_shadow_offset = { 0.0f, 0.0f };
	float countdown_scale = 1.0f;

	// The control legend shown under the countdown, for the three seconds
	// before every match. What it says is in C++ beside the code that reads
	// the pad, because a legend that disagrees with the bindings is worse than
	// none; how it looks is here.
	//
	// Here rather than in presentation.json, which is where a line of shell
	// text belongs, because the countdown it sits under is already here and
	// splitting one moment across two files with two audiences would be worse
	// than either home. Both of them want moving, together, one day.
	std::string controls_font;
	artattack::Colour controls_colour = artattack::Colour::white;
	artattack::Colour controls_shadow_colour = artattack::Colour::black;
	mattmath::Vector2F controls_shadow_offset = { 0.0f, 0.0f };

	// Down the screen, as a fraction of its height, so the legend clears the
	// countdown at any resolution.
	float controls_y = 0.8f;

	SoundTuning countdown_sound;
	SoundTuning zoom_out_sound;
};

// One animation clip, and how fast it runs.
//
// `frame_time` is optional in the file: absent means the strip's own default,
// which is what the sprite sheet already says. It used to be a float member
// initialised to FLT_MIN and compared against FLT_MIN at the one place that
// read it - a sentinel standing in for "absent" in a type that had no way to
// say it. JSON does, so the sentinel is gone.
struct PlayerAnimationTuning
{
	std::string sheet;
	std::string strip;
	std::string uniform_texture;

	// Absent means "use the strip's default frame time". Not a negative
	// number standing in for absence: a negative frame time is as legal a
	// float as FLT_MIN was, and recovering "missing" by comparing against a
	// real value is the thing the sentinel was removed to stop doing.
	std::optional<float> frame_time = std::nullopt;
};

// The paint itself: how finely a painted surface is divided, how deep the
// paint sits on a face, and what a splash sounds like.
//
// These were paint_tile_consts and structure_paintable_consts - eleven
// SCREAMING constants in two headers. They are here rather than in
// presentation.json because they are balance: `size` is the granularity the
// whole scoring system measures coverage at, and halving it quadruples both
// the tile count per structure and the resolution of every percentage the
// results screen shows.
struct PaintTuning
{
	// One tile. A structure is divided into a grid of these.
	mattmath::Vector2F size = { 0.0f, 0.0f };

	// How far paint reaches in from a painted face.
	float thickness = 0.0f;

	std::string sheet;
	std::string frame;

	std::string splash_sheet;
	std::string splash_animation;

	std::string sound_bank;
	SoundTuning splash_sound;
};

struct PlayerTuning
{
	mattmath::Vector2F size = { 0.0f, 0.0f };
	mattmath::Vector2F max_velocity = { 0.0f, 0.0f };

	float x_initial_velocity = 0.0f;
	float x_acceleration = 0.0f;
	float x_deacceleration = 0.0f;

	float gravity = 0.0f;
	float ramp_gravity_multiplier = 1.0f;

	float jump_launch_velocity = 0.0f;
	float jump_acceleration = 0.0f;
	float jump_max_time = 0.0f;

	float drop_down_velocity = 0.0f;
	float drop_down_analog_threshold = 0.0f;

	float starting_health = 1.0f;
	float health_regen_delay = 0.0f;
	float health_regen_rate = 0.0f;
	float respawn_delay = 0.0f;
	float damage_sound_delay = 0.0f;

	std::string sound_bank;
	SoundTuning damage_sound;
	SoundTuning death_sound;
	SoundTuning jump_sound;
	SoundTuning respawn_sound;

	PlayerAnimationTuning idle;
	PlayerAnimationTuning walk;
	PlayerAnimationTuning running;
	PlayerAnimationTuning jumping;
	PlayerAnimationTuning falling;
};

struct WeaponTuning
{
	WeaponDetails details;

	// Every weapon has these; the ones that add none of the player's velocity
	// simply leave the two keys out and take the defaults.
	RelativeWeaponDetails relative;

	// Empty for a weapon that fires a one-shot wave instead of holding a loop,
	// which is what Weapon reads as "nothing to resolve".
	WeaponLoopSounds loop_sounds;
};

struct ProjectileTuning
{
	ProjectileDetails details;

	// `type` is no_diffusion unless the file gave a "diffusion" block.
	DiffusionDetails diffusion;

	// The file states this once, above the five records, because it is a
	// ceiling on the integrator rather than a property of any one projectile.
	// The loader copies it into each record so that a Projectile needs one
	// borrowed pointer and not two.
	mattmath::Vector2F max_velocity = { 0.0f, 0.0f };
};

// Everything ./tuning.json says, parsed once at startup.
//
// Lookup is by enum and lands on an array slot, because the enum is what the
// code has: the file's names are turned into WeaponType and projectile_type
// while it is being read, so a misspelt weapon name is a startup failure
// naming the file and the key rather than a miss at the moment a player picks
// it (T6). Nothing here is touched per frame - a weapon copies its record at
// construction, exactly as it used to copy the constant.
class GameTuning
{
public:
	// Reads `path` in full. Throws std::runtime_error naming the file, the
	// position in it and the key if anything is missing, of the wrong type, or
	// names a weapon, projectile, diffusion or velocity mode that does not
	// exist.
	explicit GameTuning(const char* path);

	const MatchTuning& match() const;
	const PaintTuning& paint() const;
	const PlayerTuning& player() const;

	// Shared across every weapon: how fast ammo comes back, and how much of it
	// a weapon starts with.
	float ammo_regen_delay() const;
	float ammo_regen_rate() const;
	float starting_ammo() const;

	// Throws std::out_of_range naming the weapon if the file tunes no such
	// one - which is what a secondary or an unimplemented pick would be.
	const WeaponTuning& weapon(WeaponType type) const;
	const ProjectileTuning& projectile(projectile_type type) const;

private:
	// Sized by the last tuned enumerator rather than by the enum's full width:
	// WeaponType carries six more names for weapons that do not exist yet, and
	// a slot for each would be five empty records and a lookup that answered
	// with one instead of throwing.
	static constexpr size_t WEAPON_COUNT =
		static_cast<size_t>(WeaponType::bucket) + 1;
	static constexpr size_t PROJECTILE_COUNT =
		static_cast<size_t>(BALL) + 1;

	MatchTuning match_;
	PaintTuning paint_;
	PlayerTuning player_;

	float ammo_regen_delay_ = 0.0f;
	float ammo_regen_rate_ = 0.0f;
	float starting_ammo_ = 1.0f;

	std::array<WeaponTuning, WEAPON_COUNT> weapons_{};
	std::array<ProjectileTuning, PROJECTILE_COUNT> projectiles_{};

	// True where the file supplied a record, so a lookup for an untuned one
	// throws rather than answering with a zeroed struct.
	std::array<bool, WEAPON_COUNT> weapon_tuned_{};
	std::array<bool, PROJECTILE_COUNT> projectile_tuned_{};
};
