#pragma once

#include "engine/math/rectanglef.h"

// Current rectangular top supports, separate from upright ramp surfaces.
struct MatchBotSupport
{
	mattmath::RectangleF bounds;
	bool one_way = false;
	bool paintable = false;
};

struct MatchBotRamp
{
	mattmath::RectangleF bounds;
	bool rises_right = false;
};
