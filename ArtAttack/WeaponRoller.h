#ifndef WEAPONROLLER_H
#define WEAPONROLLER_H

#include "Weapon.h"

class WeaponRoller : public Weapon
{
private:
	void update_movement_and_rotation(player_input input,
		const MattMath::Vector2F& player_center,
		const MattMath::Vector2F& player_velocity,
		bool player_facing_right) override;
public:
	WeaponRoller(player_team team,
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


	//std::vector<std::unique_ptr<ICollisionGameObject>>
	//	update_and_get_projectiles(player_input input,
	//		const MattMath::Vector2F& player_center,
	//		const MattMath::Vector2F& player_velocity,
	//		bool player_facing_right) override;
};



#endif // !WEAPONROLLER_H
