#pragma once

// Projectile kinds shared by weapon tuning and projectile construction.
enum class ProjectileType
{
	spray,
	jet,
	rolling,
	mist,
	ball,
	// Count of kinds, for tables and loops over them; keep new kinds before
	// this sentinel.
	count
};

// Whether Projectile::resolve_sweep tests this kind's whole travel each tick
// (Projectile::resolve_movement_contacts), so a thin wall cannot be stepped
// over between two endpoint samples. The roller is the one exception: it is a
// one-step volume that never moves. A new kind is unswept until it is listed
// here, and game/tunnelling_limits.cpp, which visits every kind before
// ProjectileType::count, then refuses to compile until it is given a bound.
constexpr bool is_swept(ProjectileType type)
{
	switch (type)
	{
	case ProjectileType::spray:
	case ProjectileType::jet:
	case ProjectileType::mist:
	case ProjectileType::ball:
		return true;
	case ProjectileType::rolling:
		return false;
	}
	return false;
}
