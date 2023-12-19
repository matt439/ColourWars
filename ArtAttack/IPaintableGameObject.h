#ifndef IPAINTABLEGAMEOBJECT_H
#define IPAINTABLEGAMEOBJECT_H

//#include "ICollisionGameObject.h"

struct paint_total
{
	float team_a = 0.0f;
	float team_b = 0.0f;
};

class IPaintableGameObject // : virtual public ICollisionGameObject
{
public:
	virtual ~IPaintableGameObject() = default;
	virtual paint_total get_paint_total() const = 0;
};

#endif // !IPAINTABLE_H
