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

	// Keep arming through the release frame and clear it on the next frame.
	// Repeated reads of the same frame then produce the same action.
	// Do not arm on reconnect: a held button has a zeroed previous state.
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

	// Accept a release only when exactly one action was down last frame,
	// so releasing conflicting actions together does not choose between them.
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

	// Use raw stick samples: a direction fires when consecutive frames cross
	// back inside the return threshold, independently of movement dead zones.
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
	const auto down = [&keyboard](const ActionKey& entry)
	{
		return keyboard.held(entry.key) || keyboard.held(entry.alternate);
	};
	const auto went_down = [&keyboard](const ActionKey& entry)
	{
		return keyboard.pressed(entry.key) || keyboard.pressed(entry.alternate);
	};

	// Recover the previous held state from the keyboard's edge queries.
	const auto was_down = [&keyboard](const ActionKey& entry)
	{
		const auto key_was_down = [&keyboard](Key key)
		{
			return (keyboard.held(key) && !keyboard.pressed(key)) ||
				keyboard.released(key);
		};
		return key_was_down(entry.key) || key_was_down(entry.alternate);
	};

	// Keep arming through the release frame, as for pads, for repeatable reads.
	// Clear it on focus return: suppressed press edges would otherwise preserve
	// ownership of a key held while the window was unfocused.
	const bool just_focused = keyboard.just_focused();
	for (int i = 0; i < action_key_count; ++i)
	{
		this->key_armed_[i] = !just_focused &&
			(went_down(action_keys[i]) ||
				(this->key_armed_[i] && was_down(action_keys[i])));
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

	// Reject simultaneous releases of different actions.
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

	// Keyboard navigation uses press edges; pad navigation uses releases
	// and stick return crossings.
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

void MenuInput::reset_pad_actions(int slot)
{
	if (slot < 0 || slot >= input_slots::pad_count)
	{
		return;
	}
	for (bool& armed : this->armed_[slot])
	{
		armed = false;
	}
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
