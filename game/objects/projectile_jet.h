#pragma once

#include "game/objects/projectile.h"
#include "engine/render/colour.h"

class ProjectileJet final : public Projectile
{
public:
	ProjectileJet() = delete;
	ProjectileJet(const labrador::RectangleF& rectangle,
		const labrador::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip strip,
		const ProjectileTuning* tuning,
		float rotation = 0.0f,
		const labrador::Vector2F& origin = labrador::Vector2F::zero,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	labrador::RectangleF bounds() const override;
	labrador::RectangleF cull_bounds(float units_per_pixel) const override;

	const labrador::Shape* shape() const override;
private:
	void set_rectangle(const labrador::RectangleF& rectangle) override;
	labrador::RectangleF rectangle_ = labrador::RectangleF::zero;
};
