#ifndef IGAMEOBJECT_H
#define IGAMEOBJECT_H

//#include "shape_type.h"
//#include "IShape.h"
//#include "interface_type.h"
//#include <vector>
#include "MattMath.h"

class IGameObject
{
public:
	virtual ~IGameObject() = default;
	virtual void update() = 0;
	virtual void draw(const MattMath::Camera& camera) = 0;
	virtual void draw() = 0;
	virtual bool is_visible_in_viewport(const MattMath::RectangleF& view) const = 0;
	//virtual shape_type get_shape_type() const = 0;
	//virtual const MattMath::Shape* get_shape_const() const = 0;
	//virtual MattMath::Shape* get_shape() = 0;
	//virtual const std::vector<interface_type>& get_interface_types() const = 0;
};

#endif // !IGAMEOBJECT_H
