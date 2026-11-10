#pragma once

#include <span>

namespace labrador
{
	class CollisionObject;
}

// What a list of projectile targets answers the same way every tick, so a
// Level asks once (LevelStructures::admit) instead of once per swept shot.
// A caller that has no answers passes none, and the sweep asks the list
// itself, as it always did.
struct ProjectileTargetFlags
{
	// A target is a Structure with a mover(): travel is swept in its frame.
	bool movers = false;
	// A target is tagged structure_fixed_slab: the sweep takes the slab path.
	bool fixed_slabs = false;

	// Throws std::logic_error on a collision object that is not a Collider.
	static bool any_mover(std::span<labrador::CollisionObject* const> objects);
	static bool any_fixed_slab(std::span<labrador::CollisionObject* const> objects);
	static ProjectileTargetFlags scan(std::span<labrador::CollisionObject* const> objects)
	{
		return { any_mover(objects), any_fixed_slab(objects) };
	}

	bool operator==(const ProjectileTargetFlags&) const = default;
};
