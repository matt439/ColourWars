#ifndef WEAPONMISTER_H
#define WEAPONMISTER_H

#include "Weapon.h"

class WeaponMister final : public RelativeVelocityWeapon
{
public:
	WeaponMister(player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		wep_type type,
		const MattMath::Vector2F& player_center,
		ResourceManager* resource_manager,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
};

#endif // !WEAPONMISTER_H
