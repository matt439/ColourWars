#ifndef WEAPONBUILDER_H
#define WEAPONBUILDER_H

#include "game/objects/weapon_sprayer.h"
#include "game/objects/weapon_sniper.h"
#include "game/objects/weapon_bucket.h"
#include "game/objects/weapon_mister.h"
#include "game/objects/weapon_roller.h"

class WeaponBuilder
{
public:
	WeaponBuilder() = default;
	static std::unique_ptr<Weapon> build_weapon(
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		wep_type type,
		const MattMath::Vector2F& player_center,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const float* dt);
};

#endif // !WEAPONBUILDER_H
