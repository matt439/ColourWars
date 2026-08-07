#pragma once

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
		PlayerTeam team,
		int player_num,
		const mattmath::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const float* dt);
};
