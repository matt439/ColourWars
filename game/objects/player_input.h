#pragma once

#include "game/play_settings.h"
#include "game/input_bindings.h"

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "game/objects/slot_input.h"
#include "game/tuning.h"
#include "game/input_slots.h"
#include <array>
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
	// Keyboard aim remains unrequested until SeatCommandBuilder supplies the
	// camera-dependent direction from the player's on-screen centre to the cursor.
	// devices[slot] is the pad that slot's entry reads, for a seat a
	// controller reclaimed from another slot; that slot's bindings and stick
	// preferences still apply. Omitted, every slot reads its own pad.
	// Not const: a read refreshes the mapping copies below.
	std::vector<SlotInput> player_inputs();
	std::vector<SlotInput> player_inputs(
		const std::array<int, input_slots::pad_count>& devices);

private:
	SlotInput pad_input(const labrador::Gamepads& gamepads,
		int device, int slot);
	SlotInput keyboard_and_mouse_input(
		const labrador::Keyboard& keyboard, const labrador::Mouse& mouse);
	const ActionBindings& effective_bindings();
	const ActionBindings& effective_pad_bindings(int slot);

	const labrador::Gamepads* gamepads_ = nullptr;
	const labrador::Keyboard* keyboard_ = nullptr;
	const labrador::Mouse* mouse_ = nullptr;
	const GameTuning* tuning_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
	const InputBindings* input_bindings_ = nullptr;
	// Only changed saved keyboard choices or a changed pad profile rebuild a
	// copy; edges always come from the engine's continuously sampled
	// snapshots. Only keyboard_choices_' keyboard fields are compared.
	InputBindings keyboard_choices_;
	ActionBindings keyboard_bindings_;
	std::array<PadProfile, input_slots::pad_count> pad_profiles_{};
	std::array<ActionBindings, input_slots::pad_count> pad_bindings_;
};
