#pragma once

#include "game/objects/projectile.h"
#include "engine/render/colour.h"

class ProjectileRolling final : public Projectile
{
public:
	ProjectileRolling() = delete;
	ProjectileRolling(const mattmath::RectangleF& rectangle,
		const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip strip,
		const ProjectileTuning* tuning,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override;
	// A rolling volume paints and damages everything it touches in its one
	// collision sweep. The floor must not consume it before a nearby player.
	// Its short lifetime retires it on the following update.
	void on_structure_contact(const Structure& other,
		const mattmath::Vector2F& normal, float penetration) override;
	void on_player_contact(const Player& other,
		const mattmath::Vector2F& normal, float penetration) override;
	void draw(labrador::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
protected:
	// It never moves, so only moving surfaces passing through it are resolved.
	void resolve_travel(std::span<labrador::CollisionObject* const> objects) override;
	// Enemies between owner and muzzle belong to its ordinary contacts.
	bool launch_hits_players() const override { return false; }
	// Touching bare geometry leaves it active to reach exposed paint.
	bool duds_on_bare_cover() const override { return false; }
private:
	void set_rectangle(const mattmath::RectangleF& rectangle) override;
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
};
