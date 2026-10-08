#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_type.h"
#include "engine/math/vector2f.h"

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

// Looping fire sounds indexed by team and match position.
struct WeaponLoopSounds
{
	std::array<std::string, 4> team_a;
	std::array<std::string, 4> team_b;

	// False when the tuning record omits looping audio.
	bool present = false;

	// index is the player's 0..3 match position, independent of input slot.
	// The caller validates both index and team.
	const std::string& sound_name(PlayerTeam team, int index) const;
};

struct WeaponDetails
{
	mattmath::Vector2F offset = { 0.0f, 0.0f }; // From player centre to weapon sprite top-left.
	mattmath::Vector2F size = { 0.0f, 0.0f };
	mattmath::Vector2F nozzle_offset = { 0.0f, 0.0f }; // From weapon sprite right-centre to nozzle.
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
