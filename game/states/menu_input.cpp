#include "game/pch.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"

using namespace mattmath;
using namespace menu_input_consts;
using namespace labrador;

Direction MenuDirectionRepeat::update(const std::array<bool, 4>& held,
	float dt, float delay, float interval)
{
	Direction wanted = Direction::none;
	int count = 0;
	for (int i = 0; i < direction_key_count; ++i)
	{
		if (held[i])
		{
			wanted = direction_keys[i].direction;
			++count;
		}
	}
	if (count == 0)
	{
		if (!this->ready_)
		{
			this->repeat_ = DirectionRepeat(delay, interval);
		}
		this->ready_ = true;
		return this->repeat_.update(Direction::none, dt);
	}
	// Conflicting/diagonal input must return to neutral rather than selecting
	// a direction by table order or firing when one of the controls is released.
	if (!this->ready_ || count != 1)
	{
		this->ready_ = false;
		return Direction::none;
	}
	return this->repeat_.update(wanted, dt);
}

Direction MenuDirectionRepeat::update_pad(const GamepadState& pad,
	float dt, float delay, float interval, const InputTuning& tuning, bool use_stick)
{
	const float values[] = { -pad.left_stick.y, pad.left_stick.y,
		-pad.left_stick.x, pad.left_stick.x };
	const GamepadButton buttons[] = { GamepadButton::dpad_up,
		GamepadButton::dpad_down, GamepadButton::dpad_left, GamepadButton::dpad_right };
	std::array<bool, 4> held = {};
	for (int i = 0; i < direction_key_count; ++i)
	{
		this->stick_[i] = use_stick && values[i] >= (this->stick_[i] ?
			tuning.menu_release_threshold : tuning.menu_engage_threshold);
		held[i] = this->stick_[i] || pad.is_down(buttons[i]);
	}
	return this->update(held, dt, delay, interval);
}

void MenuInput::update_directions(float dt, float delay, float interval, bool use_stick)
{
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (!this->gamepads_->connected(slot) || this->gamepads_->just_connected(slot))
		{
			this->navigation_[slot] = MenuDirectionRepeat();
			this->directions_[slot] = Direction::none;
			continue;
		}
		this->directions_[slot] = this->navigation_[slot].update_pad(
			this->gamepads_->state(slot), dt, delay, interval, this->tuning_->input(), use_stick);
	}
	const int slot = input_slots::keyboard_and_mouse;
	if (!this->keyboard_->focused() || this->keyboard_->just_focused())
	{
		this->navigation_[slot] = MenuDirectionRepeat();
		this->directions_[slot] = Direction::none;
		return;
	}
	std::array<bool, 4> held = {};
	for (int i = 0; i < direction_key_count; ++i)
	{
		held[i] = this->keyboard_->held(direction_keys[i].key) ||
			this->keyboard_->held(direction_keys[i].alternate);
	}
	this->directions_[slot] = this->navigation_[slot].update(held, dt, delay, interval);
}

void MenuInput::reset_directions()
{
	for (int slot = 0; slot < input_slots::count; ++slot)
	{
		this->navigation_[slot] = MenuDirectionRepeat();
		this->directions_[slot] = Direction::none;
	}
}

MenuInput::MenuInput(const Gamepads* gamepads, const Keyboard* keyboard,
	const GameTuning* tuning) :
	gamepads_(gamepads), keyboard_(keyboard), tuning_(tuning)
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

	result.direction = this->directions_[slot];

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

	result.direction = this->directions_[input_slots::keyboard_and_mouse];

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
	this->navigation_[slot] = MenuDirectionRepeat();
	this->directions_[slot] = Direction::none;
	for (bool& armed : this->armed_[slot])
	{
		armed = false;
	}
}

void MenuInput::reset_actions()
{
	this->reset_directions();
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
