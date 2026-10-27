#pragma once

#include "engine/math/rectanglef.h"

// Current rectangular top supports; ramps keep their conservative obstacle
// bounds but are not advertised as flat standing surfaces.
struct MatchBotSupport
{
	mattmath::RectangleF bounds;
	bool one_way = false;
	bool paintable = false;
};
