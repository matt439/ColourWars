#include "game/pch.h"
#include "game/objects/weapon_details.h"

const std::string& WeaponLoopSounds::sound_name(PlayerTeam team,
	int index) const
{
	const std::array<std::string, 4>& names =
		team == PlayerTeam::a ? this->team_a : this->team_b;

	return names[static_cast<size_t>(index)];
}
