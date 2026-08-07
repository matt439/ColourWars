#pragma once

#include "engine/math/matt_math.h"
#include "game/objects/projectile.h"

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

struct SoundEffectInstanceWeaponDetails
{
	std::string shoot_sound_name_a0 = "";
	std::string shoot_sound_name_a1 = "";
	std::string shoot_sound_name_a2 = "";
	std::string shoot_sound_name_a3 = "";
	std::string shoot_sound_name_b0 = "";
	std::string shoot_sound_name_b1 = "";
	std::string shoot_sound_name_b2 = "";
	std::string shoot_sound_name_b3 = "";

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
