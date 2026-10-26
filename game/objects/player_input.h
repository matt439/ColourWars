#pragma once

#include "game/play_settings.h"
#include "game/input_bindings.h"

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
		const GameTuning* tuning, const PlaySettings* preferences,
		const InputBindings* input_bindings);

	// Return input_slots::count entries indexed by device slot; disconnected slots
	// contain neutral data. Jump uses a press edge; pause and debug use release edges.
	// Keyboard aim remains unrequested until Level supplies the camera-dependent
	// direction from the player's on-screen centre to the cursor.
	std::vector<PlayerInputData> player_inputs() const;

private:
	PlayerInputData pad_input(const labrador::Gamepads& gamepads,
		int slot) const;
	PlayerInputData keyboard_and_mouse_input(
		const labrador::Keyboard& keyboard, const labrador::Mouse& mouse) const;
	const ActionBindings& effective_bindings() const;
	const ActionBindings& effective_pad_bindings(int slot) const;

	const labrador::Gamepads* gamepads_ = nullptr;
	const labrador::Keyboard* keyboard_ = nullptr;
	const labrador::Mouse* mouse_ = nullptr;
	const GameTuning* tuning_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
	const InputBindings* input_bindings_ = nullptr;
	// Reading a frame is const. Only a changed saved key rebuilds this copy;
	// edges always come from the engine's continuously sampled snapshots.
	mutable labrador::Key keyboard_jump_ = labrador::Key::none;
	mutable ActionBindings keyboard_bindings_;
	mutable std::array<labrador::GamepadButton, input_slots::pad_count> pad_jump_{};
	mutable std::array<ActionBindings, input_slots::pad_count> pad_bindings_;
};
