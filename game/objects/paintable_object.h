#pragma once

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
};

class PaintableObject
{
public:
	virtual ~PaintableObject() = default;
	virtual PaintTotal paint_total() const = 0;
};
