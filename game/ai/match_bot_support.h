#pragma once

#include "engine/math/rectanglef.h"
#include <cstdint>

// Current rectangular top supports, separate from upright ramp surfaces.
struct MatchBotSupport
{
	mattmath::RectangleF bounds;
	bool one_way = false;
	bool paintable = false;
	// The level object's stable identity, nonzero; zero compares by bounds.
	// A moving support keeps its identity while its bounds change every step.
	std::uint32_t id = 0;
};

struct MatchBotRamp
{
	mattmath::RectangleF bounds;
	bool rises_right = false;
};

inline bool same_support(const MatchBotSupport& a, const MatchBotSupport& b)
{
	if (a.id != 0 || b.id != 0) { return a.id == b.id; }
	return a.bounds.x == b.bounds.x && a.bounds.y == b.bounds.y &&
		a.bounds.width == b.bounds.width && a.bounds.height == b.bounds.height;
}
