#pragma once

#include "engine/math/scalar.h"

// Whether an aim rotation, in radians, points into the left half-plane. The
// tools flip their art vertically there, and the idle drip's bead follows the
// same flip. Exactly straight up or down counts as right.
constexpr bool aim_faces_left(float rotation)
{
	return rotation > labrador::pi / 2.0f || rotation < labrador::pi / -2.0f;
}
