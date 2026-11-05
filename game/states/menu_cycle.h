#pragma once

// Stepping through a menu's choices with wrap-around: the one rule behind the
// value rows that cycle (Options, Mode, Stage Select, Weapon Select, the
// practice Tool row, the refusals file list and a profile name's letters).

#include "engine/ui/navigation.h"
#include "game/objects/weapon_type.h"

#include <cstddef>

// The index one step from `index` (in [0, count)) among `count` choices,
// forward or back, wrapping at each end.
template <typename Index>
Index wrap_step(Index index, Index count, bool forward)
{
	if (forward)
	{
		return index + 1 >= count ? Index{ 0 } : index + 1;
	}
	return index == Index{ 0 } ? count - 1 : index - 1;
}

// The same for a left/right press: right steps forward, left back, and any
// other direction leaves the index where it is.
template <typename Index>
Index wrap_step(Index index, Index count, labrador::Direction direction)
{
	if (direction != labrador::Direction::left && direction != labrador::Direction::right)
	{
		return index;
	}
	return wrap_step(index, count, direction == labrador::Direction::right);
}

// The weapon one step from `tool` among the first `choices` WeaponType values:
// TUNED_WEAPON_COUNT for the real tools, SELECTABLE_WEAPON_COUNT to include
// Weapon Select's Random.
inline WeaponType step_tool(WeaponType tool, labrador::Direction direction, std::size_t choices)
{
	return static_cast<WeaponType>(wrap_step(static_cast<int>(tool), static_cast<int>(choices), direction));
}
