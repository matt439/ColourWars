#pragma once

#include "game/objects/i_collision_game_object.h"
#include "engine/render/texture_object.h"

class Structure : public TextureObject, public ICollisionGameObject
{
public:
	Structure() = default;
	Structure(const std::string& sheet_name,
		const std::string& frame_name,
		const MattMath::RectangleF& sprite_rectangle,
		const MattMath::Shape* collision_shape,
		RenderResources* render_resources,
		collision_object_type collision_type,
		const MattMath::Colour& color = colour_consts::WHITE,
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
	void on_collision(const ICollisionGameObject* other) override;
	collision_object_type get_collision_object_type() const override;
	const MattMath::Shape* get_shape() const override;
	bool get_for_deletion() const override;
protected:
	const MattMath::RectangleF& get_rectangle() const;
private:
	collision_object_type _collision_type =
		collision_object_type::STRUCTURE;
	MattMath::RectangleF _sprite_rectangle = MattMath::RectangleF::ZERO;
	std::unique_ptr<MattMath::Shape> _collision_shape = nullptr;
};
