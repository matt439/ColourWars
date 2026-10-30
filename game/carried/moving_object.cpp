#include "game/pch.h"
// Carried from Labrador v0.2.0, which removes this type on master (730dd99,
// "A game that used one should copy the class ... into its own source").
// Shape unchanged; it lives in colourwars:: so it can never be ambiguous with
// an engine type. See docs/review/engine-reports.md, HYG#25.
#include "game/carried/moving_object.h"

using namespace mattmath;

namespace colourwars
{
	MovingObject::MovingObject(const Vector2F& velocity, float rotation) :
		velocity_(velocity),
		rotation_(rotation)
	{

	}

	const Vector2F& MovingObject::velocity() const
	{
		return this->velocity_;
	}
	float MovingObject::velocity_x() const
	{
		return this->velocity_.x;
	}
	float MovingObject::velocity_y() const
	{
		return this->velocity_.y;
	}
	void MovingObject::set_velocity(const Vector2F& velocity)
	{
		this->velocity_ = velocity;
	}
	void MovingObject::set_velocity_x(float x)
	{
		this->velocity_.x = x;
	}
	void MovingObject::set_velocity_y(float y)
	{
		this->velocity_.y = y;
	}
	void MovingObject::alter_velocity_x(float x)
	{
		this->velocity_.x += x;
	}
	void MovingObject::alter_velocity_y(float y)
	{
		this->velocity_.y += y;
	}
	float MovingObject::rotation() const
	{
		return this->rotation_;
	}
	void MovingObject::set_rotation(float rotation)
	{
		this->rotation_ = rotation;
	}
}
