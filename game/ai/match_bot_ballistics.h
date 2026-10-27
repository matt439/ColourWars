#pragma once

#include "game/tuning.h"

namespace match_bot_ballistics
{
	// Local horizontal projectile travel, before muzzle/body extents. Gravity
	// caps this at the ideal same-height ballistic range. This deliberately
	// ignores inherited motion and is not a promise of a clear or accurate hit.
	float horizontal_reach(const WeaponTuning& tool, const ProjectileTuning& projectile);
}
