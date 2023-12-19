#ifndef ICOLLISIONGAMEOBJECT_H
#define ICOLLISIONGAMEOBJECT_H

#include "IGameObject.h"
#include "collision_object_type.h"
#include "MattMath.h"
//#include "shape_type.h"

class ICollisionGameObject : public IGameObject
{
public:
	virtual ~ICollisionGameObject() = default;
	virtual bool is_colliding(const ICollisionGameObject* other) const = 0;
	virtual void on_collision(const ICollisionGameObject* other) = 0;
	virtual collision_object_type get_collision_object_type() const = 0;
	virtual const MattMath::Shape* get_shape() const = 0;
	virtual bool get_for_deletion() const = 0;

	//virtual shape_type get_shape_type() const = 0;
	//virtual const MattMath::RectangleF& get_aabb() const = 0;

};
#endif // !ICOLLISIONGAMEOBJECT_H
