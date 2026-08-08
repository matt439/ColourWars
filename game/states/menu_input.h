#pragma once

#include "engine/input/gamepads.h"
#include "engine/math/matt_math.h"
#include "engine/ui/navigation.h"
#include "game/states/menu_input_action.h"
#include <vector>

namespace menu_input_consts
{
	// The stick has to pass the first threshold and then fall back inside the
	// second for a direction to count, so a held stick is one step and not a
	// stream of them. They are equal today; they are two names because the pair
	// is a hysteresis band and collapsing them would make a stick resting on
	// the line stutter.
	constexpr float digital_presence_threshold = 0.8f;
	constexpr float digital_absence_threshold = 0.8f;
}

struct ProcessedMenuInput
{
	// artattack::Direction, not a game enum: which way the stick went is the
	// input to engine/ui's navigation walk, and there is no version of it that
	// belongs to this game.
	artattack::Direction direction = artattack::Direction::none;
	MenuInputAction action = MenuInputAction::none;
	bool connected = false;
};

// What the pads mean to a menu, one entry per slot.
//
// A mapping and nothing else, for the same reason PlayerInput is: the pads and
// the edges between frames belong to the engine (engine/input/gamepads.h). The
// polling, the previous-frame store and the prime() that used to live here were
// the second of two copies, and they had already drifted from the first.
class MenuInput
{
public:
	explicit MenuInput(const artattack::Gamepads* gamepads);

	// Returns exactly artattack::Gamepads::max_count entries, indexed by pad
	// slot. Absent pads carry neutral input.
	//
	// Every action here is a *release* edge: the button has to come back up.
	std::vector<ProcessedMenuInput> menu_inputs() const;

private:
	static ProcessedMenuInput menu_input(const artattack::Gamepads& gamepads,
		int slot);

	const artattack::Gamepads* gamepads_ = nullptr;
};
