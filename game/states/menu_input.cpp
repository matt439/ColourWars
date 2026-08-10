#include "game/pch.h"
#include "game/states/menu_input.h"

using namespace mattmath;
using namespace menu_input_consts;
using namespace artattack;

MenuInput::MenuInput(const Gamepads* gamepads) :
	gamepads_(gamepads)
{
}

ProcessedMenuInput MenuInput::menu_input(const Gamepads& gamepads, int slot)
{
	const GamepadState& now = gamepads.state(slot);
	const GamepadState& before = gamepads.previous_state(slot);

	// Which action buttons this menu has watched go down.
	//
	// A button arms on the frame it goes down, stays armed while it is down,
	// and clears only once it has settled back *up* - one frame after the
	// release, so the release edge is still judged against the arming that
	// began it. That is also what makes this cheap to run before the answer
	// below rather than after: for any one pair of frames it lands on the same
	// value however many times it runs, so a page that reads the pads through
	// more than one helper in an update gets the same answer from each.
	//
	// The frame a pad comes back is exempt. Gamepads keeps a zeroed previous_
	// for every frame a slot was empty or the reader was suspended, so anything
	// already being held then looks like a press nobody made.
	bool* armed = this->armed_[slot];
	const bool pad_just_connected = gamepads.just_connected(slot);
	for (int i = 0; i < action_button_count; ++i)
	{
		const GamepadButton button = action_buttons[i].button;
		const bool went_down = now.is_down(button) && !before.is_down(button);
		armed[i] = !pad_just_connected &&
			(went_down || (armed[i] && before.is_down(button)));
	}

	ProcessedMenuInput result;
	result.connected = true;

	// An action fires when exactly one of the three was down last frame and
	// none is down now. The "exactly one" is what stops a player holding A and
	// B together from getting an answer out of releasing both.
	const bool nothing_down =
		!now.is_down(GamepadButton::a) &&
		!now.is_down(GamepadButton::b) &&
		!now.is_down(GamepadButton::start);

	const auto only_one_was = [&before](GamepadButton wanted)
	{
		if (!before.is_down(wanted))
		{
			return false;
		}
		int down = 0;
		for (const GamepadButton button : { GamepadButton::a,
			GamepadButton::b, GamepadButton::start })
		{
			if (before.is_down(button))
			{
				++down;
			}
		}
		return down == 1;
	};

	if (nothing_down)
	{
		for (int i = 0; i < action_button_count; ++i)
		{
			if (armed[i] && only_one_was(action_buttons[i].button))
			{
				result.action = action_buttons[i].action;
				break;
			}
		}
	}

	// A direction also fires on the way back, so a held stick is one step and
	// not a stream of them: the stick has to have been outside the presence
	// threshold and be back inside the absence one. The stick is raw here, on
	// purpose - a menu wants the physical position against its own band, not
	// something already rescaled for walking speed.
	const Vector2F stick = now.left_stick;
	const Vector2F was = before.left_stick;

	const bool analog_up =
		was.y < -digital_presence_threshold &&
		stick.y > -digital_absence_threshold;
	const bool analog_down =
		was.y > digital_presence_threshold &&
		stick.y < digital_absence_threshold;
	const bool analog_left =
		was.x < -digital_presence_threshold &&
		stick.x > -digital_absence_threshold;
	const bool analog_right =
		was.x > digital_presence_threshold &&
		stick.x < digital_absence_threshold;

	const bool no_dpad_now =
		!now.is_down(GamepadButton::dpad_up) &&
		!now.is_down(GamepadButton::dpad_down) &&
		!now.is_down(GamepadButton::dpad_left) &&
		!now.is_down(GamepadButton::dpad_right);

	const auto dpad_released = [&before, no_dpad_now](GamepadButton button)
	{
		return no_dpad_now && before.is_down(button);
	};

	if (analog_up || dpad_released(GamepadButton::dpad_up))
	{
		result.direction = artattack::Direction::up;
	}
	else if (analog_down || dpad_released(GamepadButton::dpad_down))
	{
		result.direction = artattack::Direction::down;
	}
	else if (analog_left || dpad_released(GamepadButton::dpad_left))
	{
		result.direction = artattack::Direction::left;
	}
	else if (analog_right || dpad_released(GamepadButton::dpad_right))
	{
		result.direction = artattack::Direction::right;
	}

	return result;
}

std::vector<ProcessedMenuInput> MenuInput::menu_inputs()
{
	std::vector<ProcessedMenuInput> result(Gamepads::max_count);
	for (int slot = 0; slot < Gamepads::max_count; ++slot)
	{
		if (this->gamepads_->connected(slot))
		{
			result[slot] = this->menu_input(*this->gamepads_, slot);
		}
	}
	return result;
}
