#pragma once

#include "game/play_settings.h"
#include "game/action_bindings.h"

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "game/objects/player_input_data.h"
#include "game/tuning.h"
#include <vector>

// Map engine input snapshots to gameplay actions, one entry per device slot.
// The engine tracks edges continuously, including while menus are open.
class PlayerInput
{
public:
	// Borrow tuning and preferences. Read preferences each frame so pause-menu
	// settings apply immediately; tuning supplies pad thresholds.
	PlayerInput(const labrador::Gamepads* gamepads,
		const labrador::Keyboard* keyboard,
		const labrador::Mouse* mouse,
		const GameTuning* tuning, const PlaySettings* preferences);

	// Return input_slots::count entries indexed by device slot; disconnected slots
	// contain neutral data. Jump uses a press edge; pause and debug use release edges.
	// Keyboard aim remains unrequested until Level supplies the camera-dependent
	// direction from the player's on-screen centre to the cursor.
	std::vector<PlayerInputData> player_inputs() const;

private:
	PlayerInputData pad_input(const labrador::Gamepads& gamepads,
		int slot) const;
	static PlayerInputData keyboard_and_mouse_input(
		const labrador::Keyboard& keyboard, const labrador::Mouse& mouse);

	const labrador::Gamepads* gamepads_ = nullptr;
	const labrador::Keyboard* keyboard_ = nullptr;
	const labrador::Mouse* mouse_ = nullptr;
	const GameTuning* tuning_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
};
