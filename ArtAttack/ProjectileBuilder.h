#ifndef PROJECTILEBUILDER_H
#define PROJECTILEBUILDER_H

#include "Projectile.h"
#include "ProjectileSpray.h"
#include "ProjectileJet.h"
#include "ProjectileBall.h"
#include "ProjectileMist.h"
#include "ProjectileRolling.h"

class ProjectileBuilder
{
public:
	ProjectileBuilder() = default;
	std::vector<std::unique_ptr<ICollisionGameObject>> build_projectiles(
		const MattMath::Vector2F& position,
		const MattMath::Vector2F& velocity,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		projectile_type type,
		const float* dt,
		ResourceManager* resource_manager,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f) const;

private:
	static std::vector<std::unique_ptr<ICollisionGameObject>> build_mist_projectiles(
		const MattMath::Vector2F& position,
		const MattMath::Vector2F& velocity,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		projectile_type type,
		const float* dt,
		ResourceManager* resource_manager,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

};
#endif // !PROJECTILEBUILDER_H
