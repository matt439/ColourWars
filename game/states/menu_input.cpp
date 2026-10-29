#include "game/pch.h"
#include "game/states/menu_input.h"
#include "game/match_setup.h"

using namespace mattmath;
using namespace labrador;

ProcessedMenuInput menu_input_for_owner(
	const std::vector<ProcessedMenuInput>& inputs, int opening_slot,
	const MatchSetup& settings)
{
	if (opening_slot < 0 || opening_slot >= input_slots::count ||
		static_cast<size_t>(opening_slot) >= inputs.size())
	{
		return ProcessedMenuInput();
	}

	// A seat that a returning pad reclaimed (ROAD#4) reads that pad.
	const int opening_device = settings.player_index_for_slot(opening_slot) != -1 ?
		settings.device_slot(opening_slot) : opening_slot;
	if (static_cast<size_t>(opening_device) < inputs.size() &&
		inputs[static_cast<size_t>(opening_device)].connected)
	{
		return inputs[static_cast<size_t>(opening_device)];
	}

	// The opening device is disconnected. Other participants may navigate;
	// if none are connected, any device may reach the recovery actions.
	bool participant_connected = false;
	for (size_t slot = 0; slot < inputs.size(); slot++)
	{
		const ProcessedMenuInput& input = inputs[slot];
		if (input.connected &&
			settings.player_index_for_device(static_cast<int>(slot)) != -1)
		{
			participant_connected = true;
			if (input.action != MenuInputAction::none ||
				input.direction != Direction::none)
			{
				return input;
			}
		}
	}
	if (!participant_connected)
	{
		for (const ProcessedMenuInput& input : inputs)
		{
			if (input.connected && (input.action != MenuInputAction::none ||
				input.direction != Direction::none))
			{
				return input;
			}
		}
	}
	return ProcessedMenuInput();
}

Direction MenuDirectionRepeat::update(const std::array<bool, 4>& held,
	float dt, float delay, float interval)
{
	Direction wanted = Direction::none;
	int count = 0;
	const ActionBindings& bindings = default_action_bindings();
	for (int i = 0; i < menu_direction_count; ++i)
	{
		if (held[i])
		{
			wanted = bindings.directions[i].direction;
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
	const auto dpad = dpad_direction_held(default_action_bindings(), pad);
	std::array<bool, 4> held = {};
	for (int i = 0; i < menu_direction_count; ++i)
	{
		this->stick_[i] = use_stick && values[i] >= (this->stick_[i] ?
			tuning.menu_release_threshold : tuning.menu_engage_threshold);
		held[i] = this->stick_[i] || dpad[i];
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
	const auto held = keyboard_direction_held(default_action_bindings(), this->keyboard_->state());
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
	const ActionBindings& bindings = default_action_bindings();
	std::array<ActionSample, menu_action_count> actions{};
	bool anything_down = false;
	int was_down_count = 0;
	for (int i = 0; i < menu_action_count; ++i)
	{
		const ActionSample sample = menu_action(bindings, static_cast<MenuInputAction>(i), now, before);
		actions[i] = sample;
		armed[i] = !pad_just_connected &&
			(sample.pressed || (armed[i] && sample.was_held));
		anything_down = anything_down || sample.held;
		if (sample.was_held) { ++was_down_count; }
	}

	ProcessedMenuInput result;
	result.connected = true;

	// Accept a release only when exactly one action was down last frame,
	// so releasing conflicting actions together does not choose between them.
	if (!anything_down && was_down_count == 1)
	{
		for (int i = 0; i < menu_action_count; ++i)
		{
			if (armed[i] && actions[i].was_held)
			{
				result.action = static_cast<MenuInputAction>(i);
				break;
			}
		}
	}

	result.direction = this->directions_[slot];

	return result;
}

ProcessedMenuInput MenuInput::keyboard_menu_input(const Keyboard& keyboard)
{
	// Keep arming through the release frame, as for pads, for repeatable reads.
	// Clear it on focus return: suppressed press edges would otherwise preserve
	// ownership of a key held while the window was unfocused.
	const bool just_focused = keyboard.just_focused();
	const ActionBindings& bindings = default_action_bindings();
	std::array<ActionSample, menu_action_count> actions{};
	bool anything_down = false;
	int was_down_count = 0;
	for (int i = 0; i < menu_action_count; ++i)
	{
		// The evaluator reconstructs previous-held per physical key before
		// combining aliases: (held && !pressed) || released.
		const ActionSample sample = menu_action(bindings, static_cast<MenuInputAction>(i),
			keyboard.state(), keyboard.previous_state());
		actions[i] = sample;
		this->key_armed_[i] = !just_focused &&
			(sample.pressed || (this->key_armed_[i] && sample.was_held));
		anything_down = anything_down || sample.held;
		if (sample.was_held) { ++was_down_count; }
	}

	ProcessedMenuInput result;
	result.connected = true;

	// Reject simultaneous releases of different actions.
	if (!anything_down && was_down_count == 1)
	{
		for (int i = 0; i < menu_action_count; ++i)
		{
			if (this->key_armed_[i] && actions[i].was_held)
			{
				result.action = static_cast<MenuInputAction>(i);
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
	const MatchSetup& settings)
{
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	std::vector<ProcessedMenuInput> result;
	for (const SeatSetup& player : settings.player_settings())
	{
		if (player.controller == PlayerController::bot)
		{
			// A bot needs no device to keep setup ready. Only the human host
			// routes choices to its row; the bot emits no menu actions.
			result.push_back({ Direction::none, MenuInputAction::none, true });
			continue;
		}
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
