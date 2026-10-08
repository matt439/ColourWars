#include "game/pch.h"
#include "game/objects/projectile_builder.h"

using namespace mattmath;
using namespace DirectX;
using namespace labrador;

ProjectileBuilder::ProjectileBuilder(const GameTuning* tuning) :
	tuning_(tuning)
{
}

std::vector<std::unique_ptr<Projectile>>
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
	// Require a tuning record before constructing any projectile of this kind.
	const ProjectileTuning& tuning = this->tuning_->projectile(type);
	const RectangleF rectangle(position, tuning.details.col_rect_size);

	std::vector<std::unique_ptr<Projectile>> projectiles;

	switch (type)
    {
    case SPRAY:
		projectiles.push_back(
			std::make_unique<ProjectileSpray>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, &tuning, rotation, origin, flip,
				layer_depth));
		return projectiles;
    case JET:
		projectiles.push_back(
			std::make_unique<ProjectileJet>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, &tuning, rotation, origin, flip,
				layer_depth));
		return projectiles;
	case ROLLING:
		projectiles.push_back(
			std::make_unique<ProjectileRolling>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, &tuning, rotation, origin, flip,
				layer_depth));
		return projectiles;
	case MIST:
		return build_mist_projectiles(
			position, velocity, team, player_num, team_colour, &tuning,
			render_resources, rotation, origin, flip, layer_depth);
	case BALL:
		projectiles.push_back(
			std::make_unique<ProjectileBall>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, &tuning, rotation, origin, flip,
				layer_depth));
		return projectiles;
	}

	throw std::runtime_error("Projectile type " +
		std::to_string(static_cast<int>(type)) + " has no builder.");
}

std::vector<std::unique_ptr<Projectile>>
	ProjectileBuilder::build_mist_projectiles(
	const Vector2F& position,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	const ProjectileTuning* mist,
	RenderResources* render_resources,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth)
{
	// Fan five mist projectiles at multiples of pi/12 around the firing direction.
	constexpr float SPREAD[] = { 0.0f, 1.0f, -1.0f, 2.0f, -2.0f };

	const RectangleF rectangle(position, mist->details.col_rect_size);

	std::vector<std::unique_ptr<Projectile>> projectiles;
	projectiles.reserve(std::size(SPREAD));

	for (const float step : SPREAD)
	{
		projectiles.push_back(
			std::make_unique<ProjectileMist>(
				rectangle,
				Vector2F::rotate_vector(velocity, step * mattmath::PI / 12.0f),
				team,
				player_num,
				team_colour,
				render_resources,
				mist,
				rotation,
				origin,
				flip,
				layer_depth));
	}

	return projectiles;
}
