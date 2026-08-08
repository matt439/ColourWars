#include "game/pch.h"
#include "game/objects/player_input.h"

using namespace mattmath;
using namespace player_input_consts;
using namespace artattack;

PlayerInput::PlayerInput(const Gamepads* gamepads) :
	gamepads_(gamepads)
{
}

PlayerInputData PlayerInput::player_input(const Gamepads& gamepads, int slot)
{
	const GamepadState& pad = gamepads.state(slot);

	PlayerInputData result;
	result.connected = true;

	// Jump is either A or the left trigger, and the trigger is analogue, so
	// "pressed" on it means crossing the threshold rather than being past it.
	result.jump_held = gamepads.held(slot, GamepadButton::a) ||
		gamepads.trigger_held(slot, GamepadTrigger::left,
			trigger_jump_threshold);
	result.jump_pressed = gamepads.pressed(slot, GamepadButton::a) ||
		gamepads.trigger_pressed(slot, GamepadTrigger::left,
			trigger_jump_threshold);

	result.primary_shoot = gamepads.trigger_held(slot, GamepadTrigger::right,
		trigger_shoot_threshold);
	result.secondary_shoot = gamepads.held(slot,
		GamepadButton::right_shoulder);

	// Both toggles fire on the way up, not the way down. That was the existing
	// behaviour and it is deliberate rather than incidental: a menu that opens
	// under a still-held button would take the release as its own input.
	result.toggle_pause_menu = gamepads.released(slot, GamepadButton::start);
	result.toggle_debug = gamepads.released(slot, GamepadButton::back);

	result.x_movement = apply_deadzone(pad.left_stick.x, stick_deadzone);

	// Aim is a direction, so the deadzone is radial and the magnitude is
	// discarded by the normalise. Past the deadzone the vector is non-zero by
	// construction, which is the test for "the player asked to aim".
	const Vector2F aim = apply_deadzone(pad.right_stick, stick_deadzone);
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

std::vector<PlayerInputData> PlayerInput::player_inputs() const
{
	std::vector<PlayerInputData> result(Gamepads::max_count);
	for (int slot = 0; slot < Gamepads::max_count; ++slot)
	{
		if (this->gamepads_->connected(slot))
		{
			result[slot] = player_input(*this->gamepads_, slot);
		}
	}
	return result;
}
