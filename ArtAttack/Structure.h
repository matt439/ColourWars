#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "ICollisionGameObject.h"
#include "TextureObject.h"

class Structure : public TextureObject, public ICollisionGameObject
{
public:
	Structure() = default;
	Structure(const std::string& sheet_name,
		const std::string& frame_name,
		const MattMath::RectangleF& rectangle,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		collision_object_type collision_type,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update() override;
	void draw(const MattMath::Camera& camera) override;
	void draw() override;
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
	MattMath::RectangleF _rectangle = MattMath::RectangleF::ZERO;
};

#endif // !STRUCTURE_H
