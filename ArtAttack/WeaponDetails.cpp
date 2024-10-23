#include "pch.h"
#include "WeaponDetails.h"

const std::string& SoundEffectInstanceWeaponDetails::get_sound_name(
	player_team team, int index) const
{
	switch (team)
	{
	case player_team::A:
		switch (index)
		{
		case 0:
			return shoot_sound_name_a0;
		case 1:
			return shoot_sound_name_a1;
		case 2:
			return shoot_sound_name_a2;
		case 3:
			return shoot_sound_name_a3;
		default:
			throw std::exception("Invalid index");
		}
	case player_team::B:
		switch (index)
		{
		case 0:
			return shoot_sound_name_b0;
		case 1:
			return shoot_sound_name_b1;
		case 2:
			return shoot_sound_name_b2;
		case 3:
			return shoot_sound_name_b3;
		default:
			throw std::exception("Invalid index");
		}
	default:
		throw std::exception("Invalid team");
	}
}