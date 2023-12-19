#ifndef RECTANGLECOLLIDING_H
#define RECTANGLECOLLIDING_H

//#include "RectangleObject.h"
////#include "ICollidable.h"
//#include "CollidingObject.h"
//
//class RectangleColliding : public RectangleObject, public CollidingObject
//{
//private:
//	MattMath::RectangleF _collision_aabb = MattMath::RectangleF::ZERO;
//	//tile_collision_type _collision_type = tile_collision_type::NO_COLLISION;
//public:
//	RectangleColliding() {}
//	//RectangleColliding(const MattMath::RectangleF& bounding_box) :
//	//	RectangleObject(bounding_box),
//	//	_aabb(bounding_box) {}
//
//	RectangleColliding(const MattMath::RectangleF& bounding_box,
//		const MattMath::RectangleF& collision_aabb,
//		const std::vector<collidable_object_type>& collidable_object_types,
//		collidable_object_shape shape) :
//		RectangleObject(bounding_box),
//		CollidingObject(collidable_object_types, shape),
//		_collision_aabb(collision_aabb) {}
//
//	virtual const MattMath::RectangleF& get_collision_aabb() const;
//	virtual void set_collision_aabb(const MattMath::RectangleF& collison_aabb);
//
//	virtual void collision_inflate(float horizontal_amount, float vertical_amount);
//	virtual void collision_inflate(const MattMath::Vector2F& amount);
//	virtual void collision_scale_at_center(float scale);
//	virtual void collision_scale_at_center(float horizontal_scale, float vertical_scale);
//	virtual void collision_scale_at_center(const MattMath::Vector2F& scale);
//	virtual void collision_offset(float horizontal_amount, float vertical_amount);
//	virtual void collision_offset(const MattMath::Vector2F& amount);
//	virtual void collision_scale(float horizontal_amount, float vertical_amount);
//	virtual void collision_scale(const MattMath::Vector2F& amount);
//	virtual void collision_set_position(const MattMath::Vector2F& position);
//	virtual void collision_set_size(const MattMath::Vector2F& size);
//
//
//	//MattMath::RectangleF& get_collision_aabb_ref();
//	
//
//	//tile_collision_type get_collision_type() const override;
//	//void set_collision_type(tile_collision_type collision_type);
//};
#endif // !RECTANGLECOLLIDING_H
