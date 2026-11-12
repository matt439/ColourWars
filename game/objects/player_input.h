#pragma once

#include "game/config/play_settings.h"
#include "game/input/input_bindings.h"

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "game/objects/slot_input.h"
#include "game/config/tuning.h"
#include "game/input/input_slots.h"
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

	// Stop every toggle-fire pad's firing (REV#31). The match calls it when
	// play stops reading pads - a new round, Pause, Results, focus loss - so
	// firing never resumes on its own; the next Shoot press starts it again.
	void release_toggles();

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
	// Per pad slot: a toggle-fire pad's Shoot latch, and the frame (the pad's
	// two snapshots) whose press last flipped it, so a repeated read of that
	// frame does not flip it back. Two different frames cannot share both
	// snapshots and still hold a press edge.
	struct FireToggle
	{
		bool firing = false;
		bool flipped = false;
		labrador::GamepadState now;
		labrador::GamepadState before;
	};
	std::array<FireToggle, input_slots::pad_count> fire_toggles_{};
};
