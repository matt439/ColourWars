#ifndef RECTANGLECOLLIDINGMOVING_H
#define RECTANGLECOLLIDINGMOVING_H

////#include "RectangleObject.h"
////#include "ICollidableMovable.h"
//#include "RectangleColliding.h"
//#include "RectangleMoving.h"
//	
//class RectangleCollidingMoving : virtual public RectangleColliding,
//	virtual public RectangleMoving
//{
//private:
//	//float _rotation = 0.0f;
//	//MattMath::Vector2F _velocity = MattMath::Vector2F::ZERO;
//	//MattMath::Vector2F _dx = MattMath::Vector2F::ZERO;
//public:
//	RectangleCollidingMoving() {}
//	RectangleCollidingMoving(const MattMath::RectangleF& bounding_box,
//		const MattMath::RectangleF& aabb,
//		const std::vector<collidable_object_type>& collidable_object_types,
//		const std::vector<collidable_object_detail>& collidable_object_details,
//		const MattMath::Vector2F& velocity) :
//		RectangleColliding(bounding_box, aabb, collidable_object_types,
//			collidable_object_details),
//		RectangleMoving(bounding_box, velocity) {}
//	RectangleCollidingMoving(const MattMath::RectangleF& bounding_box,
//		const MattMath::RectangleF& aabb,
//		const std::vector<collidable_object_type>& collidable_object_types,
//		const std::vector<collidable_object_detail>& collidable_object_details,
//		const MattMath::Vector2F& velocity,
//		float rotation) :
//		RectangleColliding(bounding_box, aabb, collidable_object_types,
//			collidable_object_details),
//		RectangleMoving(bounding_box, velocity, rotation) {}
//
//	//const MattMath::Vector2F& get_velocity() const override;
//	//void set_velocity(const MattMath::Vector2F& velocity);
//	//MattMath::Vector2F& get_velocity_ref();
//
//	//const MattMath::Vector2F& get_dx() const override;
//	//void set_dx(const MattMath::Vector2F& dx);
//	//MattMath::Vector2F& get_dx_ref();
//
//	//float get_rotation() const override;
//	//void set_rotation(float rotation);
//};
#endif // !RECTANGLECOLLIDINGMOVING_H
