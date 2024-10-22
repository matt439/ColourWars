#include "pch.h"
#include "Structure.h"

using namespace DirectX;
using namespace MattMath;

void Structure::update()
{
	return;
}
void Structure::draw(const Camera& camera)
{
	this->TextureObject::draw(this->_rectangle, camera);
}
void Structure::draw()
{
	this->TextureObject::draw(this->_rectangle);
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
	return;
}
CollisionObjectType Structure::get_collision_object_type() const
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