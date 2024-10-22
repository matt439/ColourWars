#ifndef IPAINTABLEGAMEOBJECT_H
#define IPAINTABLEGAMEOBJECT_H

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
};

class IPaintableGameObject
{
public:
	virtual ~IPaintableGameObject() = default;
	virtual PaintTotal get_paint_total() const = 0;
};

#endif // !IPAINTABLE_H
