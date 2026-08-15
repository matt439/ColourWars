#pragma once

#include "game/objects/projectile.h"
#include "engine/render/colour.h"

class ProjectileBall final : public Projectile
{
public:
	ProjectileBall() = default;
	ProjectileBall(const mattmath::RectangleF& rectangle,
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
	void draw(labrador::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
};
