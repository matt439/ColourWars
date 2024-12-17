#ifndef PROJECTILEMIST_H
#define PROJECTILEMIST_H

#include "Projectile.h"

class ProjectileMist final : public DiffusingProjectile
{
public:
	ProjectileMist() = delete;
	ProjectileMist(const MattMath::RectangleF& rectangle,
		const MattMath::Vector2F& velocity,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		const float* dt,
		ResourceManager* resource_manager,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update() override;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const MattMath::Camera& camera) override;
	void draw(DirectX::SpriteBatch* sprite_batch) override;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;

	bool is_colliding(const ICollisionGameObject* other) const override;
	const MattMath::Shape* get_shape() const override;
private:
	MattMath::RectangleF _rectangle = MattMath::RectangleF::ZERO;
};
#endif // !PROJECTILEMIST_H
