#include "pch.h"
#include "PhysicalObject.h"

using namespace DirectX;
using namespace MattMath;

Vector2F PhysicalObject::get_position() const
{
	return this->_position;
}
float PhysicalObject::get_position_x() const
{
	return this->_position.x;
}
float PhysicalObject::get_position_y() const
{
	return this->_position.y;
}
void PhysicalObject::set_position(const Vector2F& position)
{
	this->_position = position;
}
void PhysicalObject::set_position_x(float x)
{
	this->_position.x = x;
}
void PhysicalObject::set_position_y(float y)
{
	this->_position.y = y;
}
void PhysicalObject::alter_position(const Vector2F& position)
{
	this->_position += position;
}
void PhysicalObject::alter_position_x(float x)
{
	this->_position.x += x;
}
void PhysicalObject::alter_position_y(float y)
{
	this->_position.y += y;
}
void PhysicalObject::set_position_from_top_right_origin(
	const Vector2F& top_right_origin)
{
	//Vector2F size = this->get_size();
	float width = this->get_width();
	Vector2F position = top_right_origin;
	position.x -= width;
	this->set_position(position);
}
void PhysicalObject::set_position_from_origin(const Vector2F& origin,
	const Vector2F& move)
{ 
	this->_position = origin + move;
}
Vector2F PhysicalObject::get_size() const
{
	return this->_size;
}
float PhysicalObject::get_width() const
{
	return this->_size.x;
}
float PhysicalObject::get_height() const
{
	return this->_size.y;
}
void PhysicalObject::set_size(const Vector2F& size)
{
	this->_size = size;
}
void PhysicalObject::set_width(float x)
{
	this->_size.x = x;
}
void PhysicalObject::set_height(float y)
{
	this->_size.y = y;
}
void PhysicalObject::alter_size(const Vector2F& size)
{
	this->_size += size;
}
void PhysicalObject::alter_size_x(float x)
{
	this->_size.x += x;
}
void PhysicalObject::alter_size_y(float y)
{
	this->_size.y += y;
}
void PhysicalObject::scale_size(const Vector2F& scale)
{
	this->_size *= scale;
}
//const RectangleF& PhysicalObject::get_bounding_box() const
//{
//	return RectangleF(this->_position, this->_size);
//}
RectangleI PhysicalObject::get_bounding_box_i() const
{
	return RectangleI(this->_position, this->_size);
}
////DirectX::SimpleMath::Rectangle PhysicalObject::get_sm_bounding_box() const
////{
////	return this->get_bounding_box().get_sm_rectangle();
////}
////RECT PhysicalObject::get_win_rect() const
////{
////	return this->get_bounding_box().get_win_rect();
////}
Vector2F PhysicalObject::get_center() const
{
	return this->_position + this->_size / 2.0f;
}