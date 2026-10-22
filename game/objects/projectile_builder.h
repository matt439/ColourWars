#pragma once

#include "game/objects/projectile.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/projectile_jet.h"
#include "game/objects/projectile_ball.h"
#include "game/objects/projectile_mist.h"
#include "game/objects/projectile_rolling.h"
#include "game/tuning.h"
#include "engine/render/colour.h"

class ProjectileBuilder
{
public:
	// Borrowed tuning must outlive the projectiles. Resolve the type record once
	// per shot; each type's sheet and strip are resolved here, once, against
	// render_resources, which every build_projectiles call must also be given.
	ProjectileBuilder(const GameTuning* tuning,
		const labrador::RenderResources* render_resources);

	// Transfers non-null projectiles to the weapon, then Player checks launch
	// obstruction before Level transfers ownership to Scene.
	std::vector<std::unique_ptr<Projectile>> build_projectiles(
		const mattmath::Vector2F& position,
		const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		ProjectileType type,
		labrador::RenderResources* render_resources,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f) const;

private:
	const GameTuning* tuning_ = nullptr;
	std::array<labrador::RenderResources::SheetStrip,
		static_cast<size_t>(ProjectileType::count)> strips_;

	// Fan five mist projectiles using the same tuning record.
	static std::vector<std::unique_ptr<Projectile>>
		build_mist_projectiles(
			const mattmath::Vector2F& position,
			const mattmath::Vector2F& velocity,
			PlayerTeam team,
			int player_num,
			const labrador::Colour& team_colour,
			const ProjectileTuning* mist,
			labrador::RenderResources* render_resources,
			labrador::RenderResources::SheetStrip strip,
			float rotation = 0.0f,
			const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
			labrador::SpriteFlip flip = labrador::SpriteFlip::none,
			float layer_depth = 0.0f);
};
