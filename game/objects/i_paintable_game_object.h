#pragma once

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
};

class IPaintableGameObject
{
public:
	virtual ~IPaintableGameObject() = default;
	virtual PaintTotal paint_total() const = 0;
};
