#pragma once

#include "game/objects/projectile.h"
#include "engine/render/colour.h"

class ProjectileRolling final : public Projectile
{
public:
	ProjectileRolling() = default;
	ProjectileRolling(const mattmath::RectangleF& rectangle,
		const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		const ProjectileTuning* tuning,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override;
	void on_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;
	void draw(labrador::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
private:
	void set_center(const mattmath::Vector2F& center) override;
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
};
