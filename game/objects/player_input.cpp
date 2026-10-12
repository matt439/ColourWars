#include "game/pch.h"
#include "game/objects/player_input.h"
#include "game/input_slots.h"

using namespace mattmath;
using namespace player_input_consts;
using namespace labrador;

PlayerInput::PlayerInput(const Gamepads* gamepads, const Keyboard* keyboard,
	const Mouse* mouse, const GameTuning* tuning, const PlaySettings* preferences) :
	gamepads_(gamepads), keyboard_(keyboard), mouse_(mouse), tuning_(tuning), preferences_(preferences)
{
}

PlayerInputData PlayerInput::pad_input(const Gamepads& gamepads, int slot) const
{
	const GamepadState& pad = gamepads.state(slot);

	PlayerInputData result;
	result.connected = true;

	// Jump uses A or the left trigger crossing its threshold. Suppress press
	// edges on reconnect: the previous state is neutral, so an already-held
	// button would otherwise look newly pressed. Preserve the held state.
	result.jump_held = gamepads.held(slot, GamepadButton::a) ||
		gamepads.trigger_held(slot, GamepadTrigger::left,
			trigger_jump_threshold);
	result.jump_pressed = !gamepads.just_connected(slot) &&
		(gamepads.pressed(slot, GamepadButton::a) ||
			gamepads.trigger_pressed(slot, GamepadTrigger::left,
				trigger_jump_threshold));

	result.primary_shoot = gamepads.trigger_held(slot, GamepadTrigger::right,
		trigger_shoot_threshold);

	// Open menus on release so the opening button is no longer held.
	// MenuInput arming also handles other buttons held during the transition.
	result.toggle_pause_menu = gamepads.released(slot, GamepadButton::start);
	result.toggle_debug = gamepads.released(slot, GamepadButton::back);
	result.skip_intro = gamepads.released(slot, GamepadButton::a) ||
		gamepads.released(slot, GamepadButton::b) ||
		gamepads.released(slot, GamepadButton::start);

	result.x_movement = apply_deadzone(pad.left_stick.x, stick_deadzone);

	// Map the downward stick threshold to a device-independent drop action.
	result.drop_through = pad.left_stick.y >
		this->tuning_->player().drop_down_analog_threshold;

	// Apply a radial deadzone, then retain only the non-zero aim direction.
	const Vector2F aim = apply_deadzone(pad.right_stick, this->preferences_->aim_deadzone());
	result.shoot_direction_requested = aim != Vector2F::ZERO;
	if (result.shoot_direction_requested)
	{
		result.shoot_direction = aim.normalized();
		result.shoot_angle = result.shoot_direction.angle();
	}

	result.left_analog_stick = pad.left_stick;
	result.right_analog_stick = pad.right_stick;

	return result;
}

PlayerInputData PlayerInput::keyboard_and_mouse_input(const Keyboard& keyboard,
	const Mouse& mouse)
{
	PlayerInputData result;
	result.connected = true;

	// Opposing movement keys cancel.
	const float left = keyboard.held(move_left_key) ? 1.0f : 0.0f;
	const float right = keyboard.held(move_right_key) ? 1.0f : 0.0f;
	result.x_movement = right - left;

	result.jump_held = keyboard.held(jump_key);
	result.jump_pressed = keyboard.pressed(jump_key);

	result.primary_shoot = mouse.held(shoot_button);

	// Dropping is a held request, checked when the player is on a pass-through platform.
	result.drop_through = keyboard.held(drop_key) ||
		keyboard.held(drop_key_alternate);

	// Match the pad's release edges so the opening key is no longer held.
	result.toggle_pause_menu = keyboard.released(pause_key);
	result.toggle_debug = keyboard.released(debug_key);
	result.skip_intro = keyboard.released(jump_key) || keyboard.released(pause_key) ||
		keyboard.released(confirm_key);

	// Level fills mouse aim after converting the player centre to screen coordinates.

	// Leave raw stick positions zero; the debug overlay should not invent stick input.

	return result;
}

std::vector<PlayerInputData> PlayerInput::player_inputs() const
{
	std::vector<PlayerInputData> result(input_slots::count);
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (this->gamepads_->connected(slot))
		{
			result[slot] = this->pad_input(*this->gamepads_, slot);
		}
	}

	// Always expose the keyboard/mouse slot, independent of recent activity.
	result[input_slots::keyboard_and_mouse] =
		keyboard_and_mouse_input(*this->keyboard_, *this->mouse_);

	return result;
}
