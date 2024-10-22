#ifndef WEAPONBUCKET_H
#define WEAPONBUCKET_H

#include "Weapon.h"

class WeaponBucket : public RelativeVelocityWeapon
{
public:
	WeaponBucket(player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		wep_type type,
		const MattMath::Vector2F& player_center,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

protected:
	void handle_shoot_sound(bool shooting_this_update, bool holding_shoot) override;
};

#endif // !WEAPONBUCKET_H
