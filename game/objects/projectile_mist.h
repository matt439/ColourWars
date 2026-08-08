#pragma once

#include "game/objects/projectile.h"
#include "engine/render/colour.h"

class ProjectileMist final : public DiffusingProjectile
{
public:
	ProjectileMist() = delete;
	ProjectileMist(const mattmath::RectangleF& rectangle,
		const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const artattack::Colour& team_colour,
		artattack::RenderResources* render_resources,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override;
	void draw(artattack::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
};
