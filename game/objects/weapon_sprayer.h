#pragma once

#include "game/objects/weapon.h"

class WeaponSprayer final : public RelativeVelocityWeapon
{
public:
	WeaponSprayer(player_team team,
		int player_num,
		const mattmath::Colour& team_colour,
		wep_type type,
		const mattmath::Vector2F& player_center,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const float* dt,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
};
