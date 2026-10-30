#include "game/pch.h"
#include "game/action_bindings.h"
#include "game/tuning.h"

using namespace labrador;

namespace
{
	void merge(ActionSample& result, const ActionSample& source)
	{
		result.held = result.held || source.held;
		result.pressed = result.pressed || source.pressed;
		result.released = result.released || source.released;
		result.was_held = result.was_held || source.was_held;
	}

	ActionSample key_sample(const KeyboardState& now,
		const KeyboardState& before, Key key)
	{
		if (!now.focused)
		{
			return {};
		}
		return { now.is_down(key), pressed(now, before, key),
			released(now, before, key), before.focused && before.is_down(key) };
	}

	ActionSample mouse_sample(const MouseState& now,
		const MouseState& before, MouseButton button)
	{
		if (!now.focused)
		{
			return {};
		}
		return { now.is_down(button), pressed(now, before, button),
			released(now, before, button), before.focused &&
				now.button_epoch == before.button_epoch && before.is_down(button) };
	}

	ActionSample button_sample(const GamepadState& now,
		const GamepadState& before, GamepadButton button)
	{
		if (!now.connected)
		{
			return {};
		}
		return { now.is_down(button), pressed(now, before, button),
			released(now, before, button), before.connected && before.is_down(button) };
	}

	float trigger_threshold(ActionTriggerThreshold selector, const InputTuning& tuning)
	{
		switch (selector)
		{
		case ActionTriggerThreshold::jump:
			return tuning.trigger_jump_threshold;
		case ActionTriggerThreshold::shoot:
			return tuning.trigger_shoot_threshold;
		default:
			throw std::invalid_argument("Unknown action trigger threshold");
		}
	}

	ActionSample trigger_sample(const GamepadState& now,
		const GamepadState& before, const TriggerActionBinding& binding,
		const InputTuning& tuning)
	{
		const float threshold = trigger_threshold(binding.threshold, tuning);
		if (!now.connected)
		{
			return {};
		}
		return { trigger_held(now, binding.trigger, threshold),
			trigger_pressed(now, before, binding.trigger, threshold),
			trigger_released(now, before, binding.trigger, threshold),
			before.connected && trigger_held(before, binding.trigger, threshold) };
	}
}

const ActionBindings& fixed_action_bindings()
{
	static const ActionBindings bindings = []
	{
		using namespace player_input_consts;
		ActionBindings result;
		const auto desktop = [&result](GameAction action) -> DesktopActionBinding&
		{
			return result.game_desktop[static_cast<std::size_t>(action)];
		};
		const auto pad = [&result](GameAction action) -> PadActionBinding&
		{
			return result.game_pad[static_cast<std::size_t>(action)];
		};

		// Escape and Down sit beside the keyboard's Pause and Drop choices.
		desktop(GameAction::drop).keys = { drop_key_alternate };
		desktop(GameAction::pause).keys = { pause_key };
		desktop(GameAction::debug).keys = { debug_key };
		desktop(GameAction::skip_intro).keys = { jump_key, pause_key, confirm_key };

		pad(GameAction::debug).buttons = { GamepadButton::back };
		pad(GameAction::skip_intro).buttons =
			{ GamepadButton::a, GamepadButton::b, GamepadButton::start };

		result.menu_keys[static_cast<std::size_t>(MenuInputAction::proceed)] =
			{ Key::enter, Key::space };
		result.menu_keys[static_cast<std::size_t>(MenuInputAction::back)] =
			{ Key::escape, Key::backspace };
		result.menu_keys[static_cast<std::size_t>(MenuInputAction::shuffle)] = { Key::r };

		result.menu_buttons[static_cast<std::size_t>(MenuInputAction::proceed)] =
			{ GamepadButton::a };
		result.menu_buttons[static_cast<std::size_t>(MenuInputAction::back)] =
			{ GamepadButton::b };
		result.menu_buttons[static_cast<std::size_t>(MenuInputAction::pause)] =
			{ GamepadButton::start };
		result.menu_buttons[static_cast<std::size_t>(MenuInputAction::shuffle)] =
			{ GamepadButton::x };

		result.directions = {
			MenuDirectionBinding{ Direction::up, { Key::up, Key::w }, GamepadButton::dpad_up },
			MenuDirectionBinding{ Direction::down, { Key::down, Key::s }, GamepadButton::dpad_down },
			MenuDirectionBinding{ Direction::left, { Key::left, Key::a }, GamepadButton::dpad_left },
			MenuDirectionBinding{ Direction::right, { Key::right, Key::d }, GamepadButton::dpad_right }
		};
		return result;
	}();
	return bindings;
}

ActionSample game_action(const ActionBindings& bindings, GameAction action,
	const KeyboardState& keyboard, const KeyboardState& previous_keyboard,
	const MouseState& mouse, const MouseState& previous_mouse)
{
	const DesktopActionBinding& binding =
		bindings.game_desktop.at(static_cast<std::size_t>(action));
	ActionSample result;
	for (Key key : binding.keys)
	{
		merge(result, key_sample(keyboard, previous_keyboard, key));
	}
	for (MouseButton button : binding.mouse_buttons)
	{
		merge(result, mouse_sample(mouse, previous_mouse, button));
	}
	return result;
}

ActionSample game_action(const ActionBindings& bindings, GameAction action,
	const GamepadState& pad, const GamepadState& previous_pad, const InputTuning& tuning)
{
	const PadActionBinding& binding =
		bindings.game_pad.at(static_cast<std::size_t>(action));
	ActionSample result;
	for (GamepadButton button : binding.buttons)
	{
		merge(result, button_sample(pad, previous_pad, button));
	}
	for (const TriggerActionBinding& trigger : binding.triggers)
	{
		merge(result, trigger_sample(pad, previous_pad, trigger, tuning));
	}
	return result;
}

ActionSample menu_action(const ActionBindings& bindings, MenuInputAction action,
	const KeyboardState& keyboard, const KeyboardState& previous_keyboard)
{
	const auto& keys = bindings.menu_keys.at(static_cast<std::size_t>(action));
	ActionSample result;
	for (Key key : keys)
	{
		ActionSample source = key_sample(keyboard, previous_keyboard, key);
		// Reconstruct each key before merging aliases: one held alias must not
		// erase another alias's newly pressed or released edge.
		source.was_held = (source.held && !source.pressed) || source.released;
		merge(result, source);
	}
	return result;
}

ActionSample menu_action(const ActionBindings& bindings, MenuInputAction action,
	const GamepadState& pad, const GamepadState& previous_pad)
{
	const auto& buttons = bindings.menu_buttons.at(static_cast<std::size_t>(action));
	ActionSample result;
	if (!pad.connected)
	{
		return result;
	}
	for (GamepadButton button : buttons)
	{
		ActionSample source = button_sample(pad, previous_pad, button);
		// The menu reader owns reconnect suppression; preserve its previous
		// button-level query even for a manually supplied disconnected frame.
		source.was_held = previous_pad.is_down(button);
		merge(result, source);
	}
	return result;
}

std::array<bool, menu_direction_count> keyboard_direction_held(
	const ActionBindings& bindings, const KeyboardState& keyboard)
{
	std::array<bool, menu_direction_count> result = {};
	if (keyboard.focused)
	{
		for (std::size_t i = 0; i < result.size(); ++i)
		{
			for (Key key : bindings.directions[i].keys)
			{
				result[i] = result[i] || keyboard.is_down(key);
			}
		}
	}
	return result;
}

std::array<bool, menu_direction_count> dpad_direction_held(
	const ActionBindings& bindings, const GamepadState& pad)
{
	std::array<bool, menu_direction_count> result = {};
	if (pad.connected)
	{
		for (std::size_t i = 0; i < result.size(); ++i)
		{
			result[i] = pad.is_down(bindings.directions[i].button);
		}
	}
	return result;
}
