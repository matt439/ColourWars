#pragma once

#include "engine/math/vector2f.h"

#include <cmath>

// Rotating a vector by an angle, which two files here need and the engine no
// longer offers.
//
// This was mattmath::Vector2F::rotate_vector until the engine's math review,
// which deleted it along with twenty-odd other names on the evidence that
// nothing called them. Nothing in *that* repository called them; weapon.cpp
// and projectile_builder.cpp call this one, and a symbol's callers cannot be
// counted from one of the two repositories - which is a sentence the deleting
// commit writes itself, about two other symbols it got wrong the same way.
//
// It is a header of its own holding a single free function rather than a
// game-side maths bag, because a bag is what accretes: the engine's own
// inflate.h is one function for the same reason, and its conventions say in
// as many words that the exception is not a licence.
namespace game_math
{
	// Counter-clockwise in maths axes, clockwise on screen, because y grows
	// downward here. Both call sites are screen-space and want the second
	// reading.
	inline mattmath::Vector2F rotate_vector(const mattmath::Vector2F& vec,
		float angle)
	{
		const float cos_angle = std::cos(angle);
		const float sin_angle = std::sin(angle);

		return mattmath::Vector2F(vec.x * cos_angle - vec.y * sin_angle,
			vec.x * sin_angle + vec.y * cos_angle);
	}
}
