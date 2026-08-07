#ifndef WEAPONSNIPER_H
#define WEAPONSNIPER_H

#include "game/objects/weapon.h"

class WeaponSniper final : public Weapon
{
public:
	WeaponSniper(player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		wep_type type,
		const MattMath::Vector2F& player_center,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
protected:
	void handle_shoot_sound(bool shooting_this_update, bool holding_shoot) override;
};

#endif // !WEAPONSNIPER_H
