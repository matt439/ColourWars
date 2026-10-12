#pragma once

#include "game/play_settings.h"

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "game/objects/player_input_data.h"
#include "game/tuning.h"
#include <vector>

namespace player_input_consts
{
	constexpr float trigger_shoot_threshold = 0.6f;
	constexpr float trigger_jump_threshold = 0.6f;

	// Scalar movement deadzone, rescaled after applying the threshold.
	// Aim uses the separate radial deadzone in saved preferences.
	constexpr float stick_deadzone = 0.5f;

	// Fixed gameplay bindings for the keyboard/mouse slot.
	constexpr labrador::Key move_left_key = labrador::Key::a;
	constexpr labrador::Key move_right_key = labrador::Key::d;
	constexpr labrador::Key jump_key = labrador::Key::space;

	// S or Down requests dropping through a platform.
	constexpr labrador::Key drop_key = labrador::Key::s;
	constexpr labrador::Key drop_key_alternate = labrador::Key::down;
	constexpr labrador::Key pause_key = labrador::Key::escape;
	constexpr labrador::Key debug_key = labrador::Key::f1;
	constexpr labrador::Key confirm_key = labrador::Key::enter;

	constexpr labrador::MouseButton shoot_button = labrador::MouseButton::left;
}

// Map engine input snapshots to gameplay actions, one entry per device slot.
// The engine tracks edges continuously, including while menus are open.
class PlayerInput
{
public:
	// Borrow tuning and preferences. Read preferences each frame so pause-menu
	// settings apply immediately; tuning supplies the drop-through threshold.
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
