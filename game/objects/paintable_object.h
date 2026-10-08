#pragma once

#include "game/input_slots.h"
#include <array>

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
	// Held area credited to each input slot, not its index in the match roster.
	std::array<float, input_slots::count> player_areas{};

	// All paintable area, including neutral tiles, for the arena coverage score.
	float total = 0.0f;
};

// A scoring role independent of the object's collision/rendering hierarchy.
// Report current held area, including neutral area in total, not paint events.
// Level discovers this role on collision objects tagged structure_paintable;
// implementing this interface alone does not enroll an object in scoring.
class PaintableObject
{
public:
	virtual ~PaintableObject() = default;
	virtual PaintTotal paint_total() const = 0;
};
