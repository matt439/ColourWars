#pragma once

#include "game/input_slots.h"
#include <array>

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
	std::array<float, input_slots::count> player_areas{};

	// All paintable area, including neutral tiles, for the arena coverage score.
	float total = 0.0f;
};

class PaintableObject
{
public:
	virtual ~PaintableObject() = default;
	virtual PaintTotal paint_total() const = 0;
};
