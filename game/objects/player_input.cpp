#include "game/pch.h"
#include "game/objects/player_input.h"
#include "game/input_slots.h"

using namespace mattmath;
using namespace player_input_consts;
using namespace labrador;

PlayerInput::PlayerInput(const Gamepads* gamepads, const Keyboard* keyboard,
	const Mouse* mouse, const GameTuning* tuning) :
	gamepads_(gamepads), keyboard_(keyboard), mouse_(mouse), tuning_(tuning)
{
}

PlayerInputData PlayerInput::pad_input(const Gamepads& gamepads, int slot) const
{
	const GamepadState& pad = gamepads.state(slot);

	PlayerInputData result;
	result.connected = true;

	// Jump is either A or the left trigger, and the trigger is analogue, so
	// "pressed" on it means crossing the threshold rather than being past it.
	//
	// The press edge is dropped on the frame a pad comes back, and the held
	// state is not. Gamepads keeps a zeroed previous_ for every frame a slot
	// was empty or the reader was suspended (engine/input/gamepad_reader.h), so
	// a button already held when a wireless pad re-associates, or when the
	// window regains the foreground, reads as a press nobody made and the
	// character jumps unasked. That a background-held button is not waiting to
	// be delivered is true of the held state, which is honest on that frame,
	// and false of the edge.
	result.jump_held = gamepads.held(slot, GamepadButton::a) ||
		gamepads.trigger_held(slot, GamepadTrigger::left,
			trigger_jump_threshold);
	result.jump_pressed = !gamepads.just_connected(slot) &&
		(gamepads.pressed(slot, GamepadButton::a) ||
			gamepads.trigger_pressed(slot, GamepadTrigger::left,
				trigger_jump_threshold));

	result.primary_shoot = gamepads.trigger_held(slot, GamepadTrigger::right,
		trigger_shoot_threshold);

	// Both toggles fire on the way up, not the way down. That was the existing
	// behaviour and it is deliberate rather than incidental: a menu that opens
	// under a still-held button would take the release as its own input. It is
	// half of that defence and only half - it says nothing about the *other*
	// buttons a hand is on when it presses Start, and MenuInput::armed_ is the
	// half that does.
	result.toggle_pause_menu = gamepads.released(slot, GamepadButton::start);
	result.toggle_debug = gamepads.released(slot, GamepadButton::back);

	result.x_movement = apply_deadzone(pad.left_stick.x, stick_deadzone);

	// Pushed down past the tuned threshold. Player used to make this
	// comparison itself off the raw stick, which left the action unreachable
	// by anything without one.
	result.drop_through = pad.left_stick.y >
		this->tuning_->player().drop_down_analog_threshold;

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

PlayerInputData PlayerInput::keyboard_and_mouse_input(const Keyboard& keyboard,
	const Mouse& mouse)
{
	PlayerInputData result;
	result.connected = true;

	// Both keys at once is neither, which is what a stick held at centre says
	// and what a player mashing to turn round expects. Summing rather than
	// branching gets that for free.
	const float left = keyboard.held(move_left_key) ? 1.0f : 0.0f;
	const float right = keyboard.held(move_right_key) ? 1.0f : 0.0f;
	result.x_movement = right - left;

	// Full deflection or nothing. A key has no travel, so there is no deadzone
	// to apply and nothing to rescale - the two operations the pad path needs
	// are both answers to a question a keyboard does not ask.
	result.jump_held = keyboard.held(jump_key);
	result.jump_pressed = keyboard.pressed(jump_key);

	result.primary_shoot = mouse.held(shoot_button);

	// Held, not pressed: the pad's version is a stick position, which is a
	// state, and Player asks about it on whichever frame the character is
	// standing on something it can fall through.
	result.drop_through = keyboard.held(drop_key) ||
		keyboard.held(drop_key_alternate);

	// Release edges, matching the pad's Start and Back for the reason the pad
	// path gives: a menu that opens under a still-held key would take the
	// release as its own input.
	result.toggle_pause_menu = keyboard.released(pause_key);
	result.toggle_debug = keyboard.released(debug_key);

	// shoot_direction is deliberately left neutral. See player_input.h: the
	// cursor means nothing until somebody knows where this player is drawn.

	// left_analog_stick and right_analog_stick stay zero. They are the raw
	// stick positions, read by the debug overlay to show what a pad is doing,
	// and there is no honest number to put in them here - a keyboard has no
	// stick position, and synthesising one would make the overlay lie about
	// which device is driving.

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

	// Always connected, with no test to pass. A keyboard is not plugged in and
	// out, and a slot that appeared the first time somebody touched a key
	// would be a rule nobody could see until it fired.
	result[input_slots::keyboard_and_mouse] =
		keyboard_and_mouse_input(*this->keyboard_, *this->mouse_);

	return result;
}
