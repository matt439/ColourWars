#pragma once

#include "game/objects/i_collision_game_object.h"
#include "engine/render/texture_object.h"

class Structure : public artattack::TextureObject, public ICollisionGameObject
{
public:
	Structure() = default;
	Structure(const std::string& sheet_name,
		const std::string& frame_name,
		const mattmath::RectangleF& sprite_rectangle,
		const mattmath::Shape* collision_shape,
		artattack::RenderResources* render_resources,
		CollisionObjectType collision_type,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update() override;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera) const override;
	bool is_visible_in_viewport(const mattmath::RectangleF& view) const override;

	bool is_colliding(const ICollisionGameObject* other) const override;
	void on_collision(const ICollisionGameObject* other) override;
	CollisionObjectType collision_object_type() const override;
	const mattmath::Shape* shape() const override;
	bool for_deletion() const override;
protected:
	const mattmath::RectangleF& rectangle() const;
private:
	CollisionObjectType collision_type_ =
		CollisionObjectType::structure;
	mattmath::RectangleF sprite_rectangle_ = mattmath::RectangleF::ZERO;
	std::unique_ptr<mattmath::Shape> collision_shape_ = nullptr;
};
