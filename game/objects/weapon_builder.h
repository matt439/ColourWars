#pragma once

#include "game/objects/weapon_sniper.h"
#include "game/objects/weapon_bucket.h"
#include "game/objects/weapon_roller.h"
#include "game/tuning.h"
#include "engine/render/colour.h"

class WeaponBuilder
{
public:
	WeaponBuilder() = default;

	// Three subclasses and two plain RelativeVelocityWeapons.
	//
	// WeaponSprayer and WeaponMister used to be classes too, and each was a
	// constructor forwarding its arguments to the base with one tuning
	// constant bound in. The constant is a record in ./tuning.json now, and
	// what was left of the two classes was nothing at all - so they are gone
	// and the two picks build the base directly. The three that remain each
	// override behaviour: the sniper and the bucket fire a one-shot wave, and
	// the roller has its own aim and its own draw colour.
	static std::unique_ptr<Weapon> build_weapon(
		const GameTuning* tuning,
		PlayerTeam team,
		int player_num,
		const artattack::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources);
};
