#include "game/pch.h"
#include "game/objects/projectile_builder.h"

using namespace mattmath;
using namespace DirectX;
using namespace projectile_consts;
using namespace artattack;

std::vector<std::unique_ptr<artattack::CollisionObject>>
	ProjectileBuilder::build_projectiles(
		const Vector2F& position,
		const Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const Colour& team_colour,
		projectile_type type,
		RenderResources* render_resources,
		float rotation,
		const Vector2F& origin,
		SpriteFlip flip,
		float layer_depth) const
{
	switch (type)
    {
    case SPRAY:
	{
		std::vector<std::unique_ptr<artattack::CollisionObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileSpray>(
				RectangleF(position, DETAILS_SPRAY.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				render_resources,
				rotation,
				origin,
				flip,
				layer_depth));
		return projectiles;
	}
    case JET:
	{
		std::vector<std::unique_ptr<artattack::CollisionObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileJet>(
				RectangleF(position, DETAILS_JET.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				render_resources,
				rotation,
				origin,
				flip,
				layer_depth));
		return projectiles;
	}
	case ROLLING:
	{
		std::vector<std::unique_ptr<artattack::CollisionObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileRolling>(
				RectangleF(position, DETAILS_ROLLING.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				render_resources,
				rotation,
				origin,
				flip,
				layer_depth));
		return projectiles;
	}
	case MIST:
		return this->build_mist_projectiles(
			position,
			velocity,
			team,
			player_num,
			team_colour,
			type,
			render_resources,
			rotation,
			origin,
			flip,
			layer_depth);
	case BALL:
	{
		std::vector<std::unique_ptr<artattack::CollisionObject>> projectiles;
		projectiles.push_back(
			std::make_unique<ProjectileBall>(
				RectangleF(position, DETAILS_BALL.col_rect_size),
				velocity,
				team,
				player_num,
				team_colour,
				render_resources,
				rotation,
				origin,
				flip,
				layer_depth));
		return projectiles;
	}
	default:
		throw std::exception("Invalid projectile type");
    };
}

std::vector<std::unique_ptr<artattack::CollisionObject>>
	ProjectileBuilder::build_mist_projectiles(
	const Vector2F& position,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	projectile_type /*type*/,	// always MIST here
	RenderResources* render_resources,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth)
{
	std::vector<std::unique_ptr<artattack::CollisionObject>> projectiles;
	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			velocity,
			team,
			player_num,
			team_colour,
			render_resources,
			rotation,
			origin,
			flip,
			layer_depth));

	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			Vector2F::rotate_vector(velocity, mattmath::PI / 12.0f),
			team,
			player_num,
			team_colour,
			render_resources,
			rotation,
			origin,
			flip,
			layer_depth));

	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			Vector2F::rotate_vector(velocity, -mattmath::PI / 12.0f),
			team,
			player_num,
			team_colour,
			render_resources,
			rotation,
			origin,
			flip,
			layer_depth));

	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			Vector2F::rotate_vector(velocity, mattmath::PI / 6.0f),
			team,
			player_num,
			team_colour,
			render_resources,
			rotation,
			origin,
			flip,
			layer_depth));

	projectiles.push_back(
		std::make_unique<ProjectileMist>(
			RectangleF(position, DETAILS_MIST.col_rect_size),
			Vector2F::rotate_vector(velocity, -mattmath::PI / 6.0f),
			team,
			player_num,
			team_colour,
			render_resources,
			rotation,
			origin,
			flip,
			layer_depth));
	return projectiles;
}