#pragma once

#include "game/tuning.h"

namespace match_bot_ballistics
{
	// Local horizontal projectile travel, before muzzle/body extents. Gravity
	// caps this at the ideal same-height ballistic range. This deliberately
	// ignores inherited motion and is not a promise of a clear or accurate hit.
	float horizontal_reach(const WeaponTuning& tool, const ProjectileTuning& projectile);

	// Finite local free-flight search against a constant-velocity observation.
	// Includes the muzzle, inherited motion and normal projectile integration;
	// cover, bounces and future changes to the target's motion are not predicted.
	mattmath::Vector2F aim_direction(const WeaponTuning& tool, const ProjectileTuning& projectile,
		mattmath::Vector2F shooter_velocity, mattmath::Vector2F target_offset,
		mattmath::Vector2F target_velocity);
}
