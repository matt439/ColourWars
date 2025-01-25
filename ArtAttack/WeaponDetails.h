#ifndef WEAPON_DETAILS_H
#define WEAPON_DETAILS_H

#include "MattMath.h"
#include "Projectile.h"

enum class add_player_velocity
{
	X_AND_Y,
	X_ONLY,
	Y_ONLY,
	NONE
};

struct RelativeWeaponDetails
{
	add_player_velocity add_vel = add_player_velocity::NONE;
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

	const std::string& get_sound_name(player_team team, int index) const;
};

struct WeaponDetails
{
	MattMath::Vector2F offset = { 0.0f, 0.0f }; //from player center to weapon sprite top-left
	MattMath::Vector2F size = { 0.0f, 0.0f };
	MattMath::Vector2F nozzle_offset = { 0.0f, 0.0f }; //from right center of weapon sprite to nozzle
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

#endif // !WEAPON_DETAILS_H
