#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "ICollisionGameObject.h"
#include "TextureObject.h"

class Structure : public TextureObject, public ICollisionGameObject
{
private:
	collision_object_type _collision_type =
		collision_object_type::STRUCTURE_FLOOR;
	MattMath::RectangleF _rectangle = MattMath::RectangleF::ZERO;
protected:
	const MattMath::RectangleF& get_rectangle() const;
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
		float layer_depth = 0.0f) :
		TextureObject(sheet_name, frame_name,
			sprite_batch, resource_manager,
			color, rotation, origin, effects, layer_depth),
		_rectangle(rectangle),
		_collision_type(collision_type) {}

	virtual void update() override;
	virtual void draw(const MattMath::Camera& camera) override;
	virtual void draw() override;
	virtual bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;

	virtual bool is_colliding(const ICollisionGameObject* other) const override;
	virtual void on_collision(const ICollisionGameObject* other) override;
	virtual collision_object_type get_collision_object_type() const override;
	virtual const MattMath::Shape* get_shape() const override;
	virtual bool get_for_deletion() const override;
};

#endif // !STRUCTURE_H
