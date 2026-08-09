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
	// `tuning` is borrowed and outlives every projectile this makes - it is
	// the GameTuning main() owns. A weapon holds one of these, so the record
	// is looked up once per shot rather than once per projectile.
	explicit ProjectileBuilder(const GameTuning* tuning);

	std::vector<std::unique_ptr<artattack::CollisionObject>> build_projectiles(
		const mattmath::Vector2F& position,
		const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const artattack::Colour& team_colour,
		projectile_type type,
		artattack::RenderResources* render_resources,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f) const;

private:
	const GameTuning* tuning_ = nullptr;

	// The mister fires five at once, fanned by a fixed angle. Every one of
	// them reads the same record, which is why this takes it rather than
	// looking it up five times.
	static std::vector<std::unique_ptr<artattack::CollisionObject>>
		build_mist_projectiles(
			const mattmath::Vector2F& position,
			const mattmath::Vector2F& velocity,
			PlayerTeam team,
			int player_num,
			const artattack::Colour& team_colour,
			const ProjectileTuning* mist,
			artattack::RenderResources* render_resources,
			float rotation = 0.0f,
			const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
			artattack::SpriteFlip flip = artattack::SpriteFlip::none,
			float layer_depth = 0.0f);
};
