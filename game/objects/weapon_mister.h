#pragma once

#include "game/objects/weapon.h"
#include "engine/render/colour.h"

class WeaponMister final : public RelativeVelocityWeapon
{
public:
	WeaponMister(PlayerTeam team,
		int player_num,
		const artattack::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const artattack::Colour& color = artattack::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);
};
