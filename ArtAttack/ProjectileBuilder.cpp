#include "pch.h"
#include "ProjectileBuilder.h"

using namespace MattMath;
using namespace DirectX;
using namespace projectile_consts;

std::vector<std::unique_ptr<ICollisionGameObject>>
	ProjectileBuilder::build_projectiles(
		const Vector2F& position,
		const Vector2F& velocity,
		player_team team,
		int player_num,
		const Colour& team_colour,
		projectile_type type,
		const float* dt,
		SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		float rotation,
		const Vector2F& origin,
		SpriteEffects effects,
		float layer_depth)
{
	switch (type)
    {
    case projectile_type::SPRAY:
	{
		std::vector<std::unique_ptr<ICollisionGameObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileSpray>(
				RectangleF(position, DETAILS_SPRAY.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				dt,
				sprite_batch,
				resource_manager,
				rotation,
				origin,
				effects,
				layer_depth));
		return projectiles;
	}
    case projectile_type::JET:
	{
		std::vector<std::unique_ptr<ICollisionGameObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileJet>(
				RectangleF(position, DETAILS_JET.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				dt,
				sprite_batch,
				resource_manager,
				rotation,
				origin,
				effects,
				layer_depth));
		return projectiles;
	}
	case projectile_type::ROLLING:
	{
		std::vector<std::unique_ptr<ICollisionGameObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileRolling>(
				RectangleF(position, DETAILS_ROLLING.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				dt,
				sprite_batch,
				resource_manager,
				rotation,
				origin,
				effects,
				layer_depth));
		return projectiles;
	}
	case projectile_type::MIST:
		return this->build_mist_projectiles(
			position,
			velocity,
			team,
			player_num,
			team_colour,
			type,
			dt,
			sprite_batch,
			resource_manager,
			rotation,
			origin,
			effects,
			layer_depth);
	case projectile_type::BALL:
	{
		std::vector<std::unique_ptr<ICollisionGameObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileBall>(
				RectangleF(position, DETAILS_BALL.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				dt,
				sprite_batch,
				resource_manager,
				rotation,
				origin,
				effects,
				layer_depth));
		return projectiles;
	}
	default:
		throw std::exception("Invalid projectile type");
    };
}

std::vector<std::unique_ptr<ICollisionGameObject>>
	ProjectileBuilder::build_mist_projectiles(
	const Vector2F& position,
	const Vector2F& velocity,
	player_team team,
	int player_num,
	const Colour& team_colour,
	projectile_type type,
	const float* dt,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth)
{
	std::vector<std::unique_ptr<ICollisionGameObject>> projectiles;
	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			velocity,
			team,
			player_num,
			team_colour,
			dt,
			sprite_batch,
			resource_manager,
			rotation,
			origin,
			effects,
			layer_depth));

	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			Vector2F::rotate_vector(velocity, MattMath::PI / 12.0f),
			team,
			player_num,
			team_colour,
			dt,
			sprite_batch,
			resource_manager,
			rotation,
			origin,
			effects,
			layer_depth));

	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			Vector2F::rotate_vector(velocity, -MattMath::PI / 12.0f),
			team,
			player_num,
			team_colour,
			dt,
			sprite_batch,
			resource_manager,
			rotation,
			origin,
			effects,
			layer_depth));
	return projectiles;
}