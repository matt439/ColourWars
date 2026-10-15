#pragma once

#include "game/play_settings.h"

#include "game/objects/weapon_sniper.h"
#include "game/objects/weapon_bucket.h"
#include "game/objects/weapon_roller.h"
#include "game/tuning.h"
#include "engine/render/colour.h"

class WeaponBuilder
{
public:
	WeaponBuilder() = default;

	// Sprayer and mister use RelativeVelocityWeapon directly. Sniper and bucket
	// override firing sound; roller overrides aim and tint.
	// player_num is the projectile's paint owner, not necessarily a device slot.
	static std::unique_ptr<Weapon> build_weapon(
		const GameTuning* tuning,
		PlayerTeam team,
		int player_num,
		int sound_index,
		const labrador::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const ToolMotionPresentation* motion = nullptr);
};
