#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_type.h"
#include "engine/math/matt_math.h"

#include <array>
#include <string>

enum class AddPlayerVelocity
{
	x_and_y,
	x_only,
	y_only,
	none
};

struct RelativeWeaponDetails
{
	AddPlayerVelocity add_vel = AddPlayerVelocity::none;
	float player_vel_amount = 0.0f;
};

// The looping fire sound, one voice per player per team.
//
// Eight named members and a two-level switch over them is what this was - the
// same table wearing eight names, and a `default:` that threw on an index the
// caller had already bounds-checked. It is two arrays of four now, which is
// what the file always said it was.
struct WeaponLoopSounds
{
	std::array<std::string, 4> team_a;
	std::array<std::string, 4> team_b;

	// True when the weapon holds a loop at all. A weapon that fires a one-shot
	// wave leaves the block out of its tuning record and gets this false.
	bool present = false;

	// `index` is the player number, 0-3. Both it and `team` are checked by the
	// one caller before it asks, so this asserts nothing and simply reads.
	const std::string& sound_name(PlayerTeam team, int index) const;
};

struct WeaponDetails
{
	mattmath::Vector2F offset = { 0.0f, 0.0f }; //from player center to weapon sprite top-left
	mattmath::Vector2F size = { 0.0f, 0.0f };
	mattmath::Vector2F nozzle_offset = { 0.0f, 0.0f }; //from right center of weapon sprite to nozzle
	float shoot_interval = 0.0f;
	float starting_vel_length = 1100.0f;
	float ammo_usage = 0.0f;
	std::string sheet_name = "";
	std::string frame_name = "";
	projectile_type proj_type = SPRAY;
	std::string sound_bank_name = "";
	std::string shoot_sound_name = "";
	float shoot_sound_volume = 1.0f;
};
