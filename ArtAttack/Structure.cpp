#include "pch.h"
#include "Structure.h"

using namespace DirectX;
using namespace MattMath;

Structure::Structure(const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& rectangle,
	ResourceManager* resource_manager,
	collision_object_type collision_type,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	TextureObject(sheet_name, frame_name, resource_manager,
		color, rotation, origin, effects, layer_depth),
	_collision_type(collision_type),
	_rectangle(rectangle)
{

}

void Structure::update()
{
	
}
void Structure::draw(SpriteBatch* sprite_batch, const Camera& camera)
{
	this->TextureObject::draw(sprite_batch, this->_rectangle, camera);
}
void Structure::draw(SpriteBatch* sprite_batch)
{
	this->TextureObject::draw(sprite_batch, this->_rectangle);
}
bool Structure::is_visible_in_viewport(const RectangleF& view) const
{
	return this->_rectangle.intersects(view);
}
bool Structure::is_colliding(const ICollisionGameObject* other) const
{
	return false;
}
void Structure::on_collision(const ICollisionGameObject* other)
{
	// do nothing
}
collision_object_type Structure::get_collision_object_type() const
{
	return this->_collision_type;
}
const Shape* Structure::get_shape() const
{
	return &this->_rectangle;
}
bool Structure::get_for_deletion() const
{
	return false;
}
const RectangleF& Structure::get_rectangle() const
{
	return this->_rectangle;
}