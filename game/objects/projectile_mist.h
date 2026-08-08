#pragma once

#include "game/objects/projectile.h"

class ProjectileMist final : public DiffusingProjectile
{
public:
	ProjectileMist() = delete;
	ProjectileMist(const mattmath::RectangleF& rectangle,
		const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const mattmath::Colour& team_colour,
		artattack::RenderResources* render_resources,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update(float dt) override;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera) const override;
	mattmath::RectangleF bounds() const override;

	bool is_colliding(const CollisionObject* other) const override;
	const mattmath::Shape* shape() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
};
