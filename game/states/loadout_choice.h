#pragma once

#include "engine/ui/navigation.h"
#include "game/objects/sub_type.h"
#include "game/objects/weapon_type.h"

#include <cstddef>

// One Weapon Select row's loadout (ROAD#11): the tool, then the sub, then
// ready. Left/Right changes whichever is being chosen; A moves on and B
// steps back, so a row picks its sub with the same three controls that pick
// its tool, and P1's Up/Down stays free to move between bot rows.
class LoadoutChoice
{
public:
	enum class Step { tool, sub, ready };
	// A sub choice is a SubType index, or random_sub for Random.
	static constexpr std::size_t random_sub = sub_count;
	static constexpr std::size_t sub_choice_count = sub_count + 1;

	LoadoutChoice() = default;
	// `tool` may be WeaponType::random_primary; the sub starts concrete.
	LoadoutChoice(WeaponType tool, SubType sub);

	// Left/Right while choosing the tool or the sub; true when it changed.
	// Ready rows and other directions change nothing.
	bool cycle(labrador::Direction direction);
	// Tool to sub, sub to ready; true when the step moved.
	bool proceed();
	// Ready to sub, sub to tool; false at the tool, which has nothing to undo.
	bool back();

	Step step() const { return this->step_; }
	WeaponType tool() const { return this->tool_; }
	std::size_t sub_choice() const { return this->sub_; }
	// The concrete sub: a Random choice takes `draw` modulo the three.
	SubType resolved_sub(unsigned int draw) const;

private:
	WeaponType tool_ = WeaponType::sprayer;
	std::size_t sub_ = 0;
	Step step_ = Step::tool;
};
