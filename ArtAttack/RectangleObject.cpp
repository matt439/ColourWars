#include "pch.h"
#include "RectangleObject.h"

using namespace MattMath;

//const RectangleF& RectangleObject::get_bounding_box() const
//{
//	return this->_rectangle;
//}
//const RectangleI RectangleObject::get_bounding_box_i() const
//{
//	return this->_rectangle.get_rectangle_i();
//}

const RectangleF* RectangleObject::get_rectangle_ptr() const
{
	return this->_rectangle.get();
}
const RectangleF& RectangleObject::get_rectangle() const
{
	return *this->_rectangle;
}
void RectangleObject::set_rectangle(const RectangleF& rectangle)
{
	this->_rectangle = std::make_unique<RectangleF>(rectangle);
}
void RectangleObject::inflate(float horizontal_amount, float vertical_amount)
{
	this->_rectangle->inflate(horizontal_amount, vertical_amount);
}
void RectangleObject::inflate(const Vector2F& amount)
{
	this->_rectangle->inflate(amount);
}
void RectangleObject::scale_at_center(float scale)
{
	this->_rectangle->scale_at_center(scale);
}
void RectangleObject::scale_at_center(float horizontal_scale,
	float vertical_scale)
{
	this->_rectangle->scale_at_center(horizontal_scale, vertical_scale);
}
void RectangleObject::scale_at_center(const Vector2F& scale)
{
	this->_rectangle->scale_at_center(scale);
}
void RectangleObject::offset(float horizontal_amount, float vertical_amount)
{
	this->_rectangle->offset(horizontal_amount, vertical_amount);
}
void RectangleObject::offset(const Vector2F& amount)
{
	this->_rectangle->offset(amount);
}
void RectangleObject::scale(float horizontal_amount, float vertical_amount)
{
	this->_rectangle->scale(horizontal_amount, vertical_amount);
}
void RectangleObject::scale(const Vector2F& amount)
{
	this->_rectangle->scale(amount);
}
void RectangleObject::set_position(const Vector2F& position)
{
	this->_rectangle->set_position(position);
}
void RectangleObject::set_size(const Vector2F& size)
{
	this->_rectangle->set_size(size);
}
//const RectangleF& RectangleObject::get_bounding_box() const
//{
//	return this->_rectangle;
//}
//RectangleI RectangleObject::get_bounding_box_i() const
//{
//	return this->_rectangle.get_rectangle_i();
//}

//const RectangleF& RectangleStaticNoCollision::get_bounding_box() const
//{
//	return this->get_bounding_box();
//}
//const RectangleI RectangleStaticNoCollision::get_bounding_box_i() const
//{
//	return this->get_bounding_box_i();
//}
//
//
//
//const RectangleF& RectangleColliding::get_collision_aabb() const
//{
//	return this->_collision_aabb;
//}
//tile_collision_type RectangleColliding::get_collision_type() const
//{
//	return this->_collision_type;
//}
//RectangleF& RectangleColliding::get_collision_aabb_ref()
//{
//	return this->_collision_aabb;
//}
//void RectangleColliding::set_collision_aabb(const RectangleF& collison_aabb)
//{
//	this->_collision_aabb = collison_aabb;
//}
//void RectangleColliding::set_collision_type(tile_collision_type collision_type)
//{
//	this->_collision_type = collision_type;
//}
//
//const Vector2F& RectangleMoving::get_velocity() const
//{
//	return this->_velocity;
//}
//void RectangleMoving::set_velocity(const Vector2F& velocity)
//{
//	this->_velocity = velocity;
//}
//MattMath::Vector2F& RectangleMoving::get_velocity_ref()
//{
//	return this->_velocity;
//}
//const Vector2F& RectangleMoving::get_dx() const
//{
//	return this->_dx;
//}
//void RectangleMoving::set_dx(const Vector2F& dx)
//{
//	this->_dx = dx;
//}
//Vector2F& RectangleMoving::get_dx_ref()
//{
//	return this->_dx;
//}
//float RectangleMoving::get_rotation() const
//{
//	return this->_rotation;
//}
//void RectangleMoving::set_rotation(float rotation)
//{
//	this->_rotation = rotation;
//}
//
//
//const Vector2F& RectangleCollidingMoving::get_velocity() const
//{
//	return this->_velocity;
//}
//void RectangleCollidingMoving::set_velocity(const Vector2F& velocity)
//{
//	this->_velocity = velocity;
//}
//MattMath::Vector2F& RectangleCollidingMoving::get_velocity_ref()
//{
//	return this->_velocity;
//}
//const Vector2F& RectangleCollidingMoving::get_dx() const
//{
//	return this->_dx;
//}
//void RectangleCollidingMoving::set_dx(const Vector2F& dx)
//{
//	this->_dx = dx;
//}
//Vector2F& RectangleCollidingMoving::get_dx_ref()
//{
//	return this->_dx;
//}
//float RectangleCollidingMoving::get_rotation() const
//{
//	return this->_rotation;
//}
//void RectangleCollidingMoving::set_rotation(float rotation)
//{
//	this->_rotation = rotation;
//}