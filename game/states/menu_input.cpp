#include "game/pch.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"

using namespace mattmath;
using namespace menu_input_consts;
using namespace labrador;

MenuInput::MenuInput(const Gamepads* gamepads, const Keyboard* keyboard) :
	gamepads_(gamepads), keyboard_(keyboard)
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
		result.direction = labrador::Direction::up;
	}
	else if (analog_down || dpad_released(GamepadButton::dpad_down))
	{
		result.direction = labrador::Direction::down;
	}
	else if (analog_left || dpad_released(GamepadButton::dpad_left))
	{
		result.direction = labrador::Direction::left;
	}
	else if (analog_right || dpad_released(GamepadButton::dpad_right))
	{
		result.direction = labrador::Direction::right;
	}

	return result;
}

ProcessedMenuInput MenuInput::keyboard_menu_input(const Keyboard& keyboard)
{
	// The same arming as the pads, and for the pad's reason exactly: hold
	// Space to jump and press Escape with the other hand, and Escape's release
	// opens the pause menu while Space's release, a frame later, activates
	// whatever it opened onto. A release only answers if the press that began
	// it was seen from here.
	const auto down = [&keyboard](const ActionKey& entry)
	{
		return keyboard.held(entry.key) || keyboard.held(entry.alternate);
	};
	const auto went_down = [&keyboard](const ActionKey& entry)
	{
		return keyboard.pressed(entry.key) || keyboard.pressed(entry.alternate);
	};

	// Down on the PREVIOUS frame, which the device reports as two cases: still
	// down now without having just arrived, or up now having just left.
	const auto was_down = [&keyboard](const ActionKey& entry)
	{
		const auto key_was_down = [&keyboard](Key key)
		{
			return (keyboard.held(key) && !keyboard.pressed(key)) ||
				keyboard.released(key);
		};
		return key_was_down(entry.key) || key_was_down(entry.alternate);
	};

	// Armed on the frame it goes down, still armed while it stays down, and
	// cleared one frame after the release - so the release edge is still
	// judged against the arming that began it, and so that running this twice
	// in one frame lands on the same value both times. A page that reads the
	// keyboard through more than one helper depends on that, and clearing the
	// flag where the action fires would have broken it.
	for (int i = 0; i < action_key_count; ++i)
	{
		this->key_armed_[i] = went_down(action_keys[i]) ||
			(this->key_armed_[i] && was_down(action_keys[i]));
	}

	ProcessedMenuInput result;
	result.connected = true;

	bool anything_down = false;
	int was_down_count = 0;
	for (const ActionKey& entry : action_keys)
	{
		anything_down = anything_down || down(entry);
		if (was_down(entry))
		{
			++was_down_count;
		}
	}

	// Exactly one, for the pad's reason: releasing Enter and Escape together
	// should not hand the page an answer to pick from.
	if (!anything_down && was_down_count == 1)
	{
		for (int i = 0; i < action_key_count; ++i)
		{
			if (this->key_armed_[i] && was_down(action_keys[i]))
			{
				result.action = action_keys[i].action;
				break;
			}
		}
	}

	// A PRESS EDGE, WHERE THE PAD USES A RELEASE, and the difference is the
	// device rather than an inconsistency. The stick fires on the way back
	// because it is analogue and a held stick would otherwise be a stream of
	// steps - the two thresholds above this function are a hysteresis band. A
	// key has no band to be inside: down-now-and-up-last-frame already means
	// exactly one step per press, and waiting for the release would only make
	// the cursor answer late.
	for (const DirectionKey& entry : direction_keys)
	{
		if (keyboard.pressed(entry.key) || keyboard.pressed(entry.alternate))
		{
			result.direction = entry.direction;
			break;
		}
	}

	return result;
}

std::vector<ProcessedMenuInput> MenuInput::menu_inputs()
{
	std::vector<ProcessedMenuInput> result(input_slots::count);
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (this->gamepads_->connected(slot))
		{
			result[slot] = this->menu_input(*this->gamepads_, slot);
		}
	}

	result[input_slots::keyboard_and_mouse] =
		this->keyboard_menu_input(*this->keyboard_);

	return result;
}

std::vector<ProcessedMenuInput> MenuInput::player_inputs(
	const MenuLevelSettings& settings)
{
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	std::vector<ProcessedMenuInput> result;
	for (const MenuPlayerSettings& player : settings.player_settings())
	{
		result.push_back(player.num >= 0 &&
			static_cast<size_t>(player.num) < inputs.size() ?
			inputs[static_cast<size_t>(player.num)] : ProcessedMenuInput());
	}
	return result;
}

void MenuInput::reset_actions()
{
	for (auto& slot : this->armed_)
	{
		for (bool& armed : slot)
		{
			armed = false;
		}
	}
	for (bool& armed : this->key_armed_)
	{
		armed = false;
	}
}
