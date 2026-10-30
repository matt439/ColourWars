#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

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

// The one spelling of each kind in tuning.json (a weapon's projectile and the
// paint splash animations), indexed by ProjectileType.
inline constexpr std::array<const char*, static_cast<size_t>(ProjectileType::count)> PROJECTILE_NAMES{
	"spray", "jet", "rolling", "mist", "ball"
};

constexpr const char* projectile_name(ProjectileType type)
{
	return PROJECTILE_NAMES[static_cast<size_t>(type)];
}

// The kind a name spells; empty for any other name.
constexpr std::optional<ProjectileType> projectile_from_name(std::string_view name)
{
	for (size_t index = 0; index < PROJECTILE_NAMES.size(); ++index)
	{
		if (name == PROJECTILE_NAMES[index]) { return static_cast<ProjectileType>(index); }
	}
	return std::nullopt;
}

// Calls visit(type, name) for each kind in enum order.
template <typename Visit>
constexpr void for_each_projectile(Visit&& visit)
{
	for (size_t index = 0; index < PROJECTILE_NAMES.size(); ++index)
	{
		visit(static_cast<ProjectileType>(index), PROJECTILE_NAMES[index]);
	}
}

// Whether Projectile::resolve_travel tests this kind's whole travel each tick
// (Projectile::resolve_movement_contacts), so a thin wall cannot be stepped
// over between two endpoint samples. The roller is the one exception: it is a
// one-step volume that never moves, and ProjectileRolling overrides
// resolve_travel. A new kind is unswept until it is listed
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
