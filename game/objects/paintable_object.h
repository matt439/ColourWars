#pragma once

#include "game/input_slots.h"
#include <array>

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
	std::array<float, input_slots::count> player_areas{};

	// Every tile the object has, whichever team holds it and including the
	// ones nobody has painted. team_a + team_b is what was painted; this is
	// what there was to paint, and the difference is the only way to ask how
	// much of the arena a match covered rather than who covered more of it.
	float total = 0.0f;
};

class PaintableObject
{
public:
	virtual ~PaintableObject() = default;
	virtual PaintTotal paint_total() const = 0;
};
