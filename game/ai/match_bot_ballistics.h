#pragma once

#include "game/config/tuning.h"

#include <optional>

namespace match_bot_ballistics
{
	// Local horizontal projectile travel, before muzzle/body extents. Gravity
	// caps this at the ideal same-height ballistic range. This deliberately
	// ignores inherited motion and is not a promise of a clear or accurate hit.
	float horizontal_reach(const WeaponTuning& tool, const ProjectileTuning& projectile);

	// Finite local free-flight search against a constant-velocity observation.
	// Includes the muzzle, inherited motion and normal projectile integration;
	// cover, bounces and future changes to the target's motion are not predicted.
	labrador::Vector2F aim_direction(const WeaponTuning& tool, const ProjectileTuning& projectile,
		labrador::Vector2F shooter_velocity, labrador::Vector2F target_offset,
		labrador::Vector2F target_velocity);

	// The angle to throw a sub (ROAD#11) from the body's centre so its body
	// passes within tolerance of a target moving at constant velocity, with
	// the thrower's horizontal run added, as SubWeapon throws it. A tool that
	// throws along a fixed angle (the roller's tilt) passes it, and gets it
	// back only if that angle reaches. Empty when no throw comes close
	// enough. The same 32-trajectory search as aim_direction; cover is not
	// predicted.
	std::optional<float> throw_angle(const SubTuning& sub, labrador::Vector2F shooter_velocity,
		labrador::Vector2F target_offset, labrador::Vector2F target_velocity,
		std::optional<float> fixed_angle, float tolerance);
}
