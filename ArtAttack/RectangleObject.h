#ifndef RECTANGLEOBJECT_H
#define RECTANGLEOBJECT_H


//#include "tile_collision_type.h"
//#include "IRectangle.h"
//#include "IPhysical.h"
#include "MattMath.h"

class RectangleObject //: public IRectangle, public IPhysical
{
private:
	std::unique_ptr<MattMath::RectangleF> _rectangle = nullptr;
protected:
	virtual const MattMath::RectangleF* get_rectangle_ptr() const;
	virtual const MattMath::RectangleF& get_rectangle() const;
	virtual void set_rectangle(const MattMath::RectangleF& rectangle);

	virtual void inflate(float horizontal_amount, float vertical_amount);
	virtual void inflate(const MattMath::Vector2F& amount);
	virtual void scale_at_center(float scale);
	virtual void scale_at_center(float horizontal_scale, float vertical_scale);
	virtual void scale_at_center(const MattMath::Vector2F& scale);
	virtual void offset(float horizontal_amount, float vertical_amount);
	virtual void offset(const MattMath::Vector2F& amount);
	virtual void scale(float horizontal_amount, float vertical_amount);
	virtual void scale(const MattMath::Vector2F& amount);
	virtual void set_position(const MattMath::Vector2F& position);
	virtual void set_size(const MattMath::Vector2F& size);
public:
	RectangleObject() {}
	RectangleObject(const MattMath::RectangleF& bounds) :
		_rectangle(std::make_unique<MattMath::RectangleF>(bounds)) {}
	
	//const MattMath::RectangleF& get_bounding_box() const override;
	//const MattMath::RectangleI get_bounding_box_i() const override;



	//virtual const MattMath::RectangleF& get_bounding_box() const override;
	//virtual MattMath::RectangleI get_bounding_box_i() const override;

	//const MattMath::RectangleF& get_bounding_box() const;
	//const MattMath::RectangleI get_bounding_box_i() const;
	//MattMath::RectangleF& get_bounding_box_ref();
	//void set_bounding_box(const MattMath::RectangleF& bounding_box);
};

//class RectangleStaticNoCollision : public RectangleObject, public IPhysicalObject
//{
//public:
//	RectangleStaticNoCollision() {}
//	RectangleStaticNoCollision(const MattMath::RectangleF& bounding_box) :
//		RectangleObject(bounding_box) {}
//
//	const MattMath::RectangleF& get_bounding_box() const override;
//	const MattMath::RectangleI get_bounding_box_i() const override;
//};
//
//
//class RectangleColliding : public RectangleObject
//{
//private:
//	MattMath::RectangleF _collision_aabb = MattMath::RectangleF::ZERO;
//	tile_collision_type _collision_type = tile_collision_type::NO_COLLISION;
//public:
//	RectangleColliding() {}
//	RectangleColliding(const MattMath::RectangleF& bounding_box,
//		const MattMath::RectangleF& collision_aabb,
//		tile_collision_type collision_type) :
//		RectangleObject(bounding_box),
//		_collision_aabb(collision_aabb),
//		_collision_type(collision_type) {}
//
//	const MattMath::RectangleF& get_collision_aabb() const;
//	MattMath::RectangleF& get_collision_aabb_ref();
//	void set_collision_aabb(const MattMath::RectangleF& collison_aabb);
//
//	tile_collision_type get_collision_type() const;
//	void set_collision_type(tile_collision_type collision_type);
//};
//
//
//class RectangleStaticColliding : public RectangleColliding,
//	public IPhysicalObject, public ICollidingObject
//{
//public:
//	RectangleStaticColliding() {}
//	RectangleStaticColliding(const MattMath::RectangleF& bounding_box,
//				const MattMath::RectangleF& collision_aabb,
//				tile_collision_type collision_type) :
//		RectangleColliding(bounding_box, collision_aabb, collision_type) {}
//
//	const MattMath::RectangleF& get_bounding_box() const override;
//	const MattMath::RectangleI get_bounding_box_i() const override;
//
//	virtual const MattMath::RectangleF& get_collision_aabb() const = 0;
//	virtual tile_collision_type get_collision_type() const = 0;
//};
//
//class RectangleMoving : public RectangleObject
//{
//private:
//	float _rotation = 0.0f;
//	MattMath::Vector2F _velocity = MattMath::Vector2F::ZERO;
//	MattMath::Vector2F _dx = MattMath::Vector2F::ZERO;
//public:
//	RectangleMoving() {}
//	RectangleMoving(const MattMath::RectangleF& bounding_box,
//		const MattMath::Vector2F& velocity) :
//		RectangleObject(bounding_box),
//		_velocity(velocity) {}
//	RectangleMoving(const MattMath::RectangleF& bounding_box,
//		const MattMath::Vector2F& velocity,
//		float rotation) :
//		RectangleObject(bounding_box),
//		_velocity(velocity),
//		_rotation(rotation) {}
//
//	const MattMath::Vector2F& get_velocity() const;
//	void set_velocity(const MattMath::Vector2F& velocity);
//	MattMath::Vector2F& get_velocity_ref();
//
//	const MattMath::Vector2F& get_dx() const;
//	void set_dx(const MattMath::Vector2F& dx);
//	MattMath::Vector2F& get_dx_ref();
//
//	float get_rotation() const;
//	void set_rotation(float rotation);
//};
//
//class RectangleCollidingMoving : public virtual RectangleColliding,
//	public virtual RectangleMoving
//{
//private:
//
//public:
//	RectangleCollidingMoving() {}
//	RectangleCollidingMoving(const MattMath::RectangleF& bounding_box,
//				const MattMath::RectangleF& aabb,
//				tile_collision_type collision_type,
//				const MattMath::Vector2F& velocity) :
//		RectangleColliding(bounding_box, aabb, collision_type),
//		RectangleMoving(bounding_box, velocity) {}
//	RectangleCollidingMoving(const MattMath::RectangleF& bounding_box,
//				const MattMath::RectangleF& aabb,
//				tile_collision_type collision_type,
//				const MattMath::Vector2F& velocity,
//				float rotation) :
//		RectangleColliding(bounding_box, aabb, collision_type),
//		RectangleMoving(bounding_box, velocity, rotation) {}
//
//	const MattMath::Vector2F& get_velocity() const override;
//	void set_velocity(const MattMath::Vector2F& velocity);
//	MattMath::Vector2F& get_velocity_ref();
//
//	const MattMath::Vector2F& get_dx() const override;
//	void set_dx(const MattMath::Vector2F& dx);
//	MattMath::Vector2F& get_dx_ref();
//
//	float get_rotation() const override;
//	void set_rotation(float rotation);
//};

#endif // !RECTANGLEOBJECT_H
