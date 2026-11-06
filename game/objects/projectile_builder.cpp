#include "game/pch.h"
#include "game/objects/projectile_builder.h"

using namespace labrador;

ProjectileBuilder::ProjectileBuilder(const GameTuning* tuning,
	const RenderResources* render_resources) :
	tuning_(tuning)
{
	for (size_t type = 0; type < this->strips_.size(); ++type)
	{
		this->strips_[type] = resolve_projectile_strip(
			tuning->projectile(static_cast<ProjectileType>(type)), *render_resources);
	}
}

std::vector<std::unique_ptr<Projectile>>
	ProjectileBuilder::build_projectiles(
		const Vector2F& position,
		const Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const Colour& team_colour,
		ProjectileType type,
		RenderResources* render_resources) const
{
	// Require a tuning record before constructing any projectile of this kind.
	const ProjectileTuning& tuning = this->tuning_->projectile(type);
	const RectangleF rectangle(position, tuning.details.col_rect_size);
	const RenderResources::SheetStrip strip = this->strips_.at(static_cast<size_t>(type));

	std::vector<std::unique_ptr<Projectile>> projectiles;

	switch (type)
    {
    case ProjectileType::spray:
		projectiles.push_back(
			std::make_unique<ProjectileSpray>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, strip, &tuning));
		return projectiles;
    case ProjectileType::jet:
		projectiles.push_back(
			std::make_unique<ProjectileJet>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, strip, &tuning));
		return projectiles;
	case ProjectileType::rolling:
		projectiles.push_back(
			std::make_unique<ProjectileRolling>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, strip, &tuning));
		return projectiles;
	case ProjectileType::mist:
		return build_mist_projectiles(
			position, velocity, team, player_num, team_colour, &tuning,
			render_resources, strip);
	case ProjectileType::ball:
		projectiles.push_back(
			std::make_unique<ProjectileBall>(
				rectangle, velocity, team, player_num, team_colour,
				render_resources, strip, &tuning));
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
	RenderResources::SheetStrip strip)
{
	// Five pellets and their centre-out order are mechanics; their angle is tuning.
	constexpr float spread[] = { 0.0f, 1.0f, -1.0f, 2.0f, -2.0f };

	const RectangleF rectangle(position, mist->details.col_rect_size);

	std::vector<std::unique_ptr<Projectile>> projectiles;
	projectiles.reserve(std::size(spread));

	for (const float step : spread)
	{
		projectiles.push_back(
			std::make_unique<ProjectileMist>(
				rectangle,
				Vector2F::rotate_vector(velocity, step * mist->spread_step),
				team,
				player_num,
				team_colour,
				render_resources,
				strip,
				mist));
	}

	return projectiles;
}
