#pragma once

#include "engine/input/gamepads.h"
#include "game/objects/player_input_data.h"
#include <vector>

namespace player_input_consts
{
	constexpr float trigger_shoot_threshold = 0.6f;
	constexpr float trigger_jump_threshold = 0.6f;

	// Applied once, radially, and rescaled - see engine/input/gamepad.h. It
	// used to be applied on top of a circular deadzone the pad API had already
	// applied, without rescaling, so a stick had half its travel dead and the
	// other half started at half speed.
	constexpr float stick_deadzone = 0.5f;
}

// What the pads mean to a match, one entry per slot.
//
// This is now a mapping and nothing else: no polling, no previous-frame store,
// no priming. The engine holds the pads and the edges (engine/input/gamepads.h)
// because an edge is only right when "previous" is genuinely last frame, which
// it cannot be for a detector that only runs while gameplay is on screen.
class PlayerInput
{
public:
	explicit PlayerInput(const labrador::Gamepads* gamepads);

	// Returns exactly labrador::Gamepads::max_count entries, indexed by pad
	// slot. Absent pads carry neutral input rather than being omitted: the
	// index is a slot and every caller reads it as a player number, so a
	// vector that skipped empty slots would hand a player someone else's
	// controller.
	//
	// jump is a *press* edge; pause and debug are *release* edges.
	std::vector<PlayerInputData> player_inputs() const;

private:
	static PlayerInputData player_input(const labrador::Gamepads& gamepads,
		int slot);

	const labrador::Gamepads* gamepads_ = nullptr;
};
