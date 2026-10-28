#include "game/pch.h"
#include "game/input_bindings.h"
#include "game/save.h"
#include "engine/assets/json.h"
#include <algorithm>
#include <array>
#include <stdexcept>

using namespace labrador;

namespace
{
	// The save token is labrador::key_name; which keys Jump may take, and what a
	// player is shown for each, are this game's.
	struct KeyName
	{
		Key key;
		std::wstring_view label;
	};

	constexpr std::array<KeyName, 27> key_names{ {
		{ Key::a, L"A" }, { Key::b, L"B" }, { Key::c, L"C" }, { Key::d, L"D" }, { Key::e, L"E" },
		{ Key::f, L"F" }, { Key::g, L"G" }, { Key::h, L"H" }, { Key::i, L"I" }, { Key::j, L"J" },
		{ Key::k, L"K" }, { Key::l, L"L" }, { Key::m, L"M" }, { Key::n, L"N" }, { Key::o, L"O" },
		{ Key::p, L"P" }, { Key::q, L"Q" }, { Key::r, L"R" }, { Key::s, L"S" }, { Key::t, L"T" },
		{ Key::u, L"U" }, { Key::v, L"V" }, { Key::w, L"W" }, { Key::x, L"X" }, { Key::y, L"Y" },
		{ Key::z, L"Z" }, { Key::space, L"Space" }
	} };

	struct ControlName
	{
		PadControl control;
		std::string_view name;
		std::wstring_view label;
	};
	constexpr std::array<ControlName, 12> control_names{ {
		{ PadControl::none, "none", L"None" }, { PadControl::a, "a", L"A" }, { PadControl::b, "b", L"B" },
		{ PadControl::x, "x", L"X" }, { PadControl::y, "y", L"Y" },
		{ PadControl::left_shoulder, "lb", L"LB" }, { PadControl::right_shoulder, "rb", L"RB" },
		{ PadControl::left_trigger, "lt", L"LT" }, { PadControl::right_trigger, "rt", L"RT" },
		{ PadControl::left_stick, "ls", L"LS" }, { PadControl::right_stick, "rs", L"RS" },
		{ PadControl::start, "start", L"Start" }
	} };

	const ControlName& control_entry(PadControl control)
	{
		for (const auto& value : control_names) { if (value.control == control) { return value; } }
		throw std::invalid_argument("Unknown pad control");
	}

	constexpr std::array<const char*, pad_action_count> action_names{ "jump", "shoot", "pause" };

	// Adds one control to an action's sources. Triggers are thresholded; a
	// shoot trigger keeps the shoot threshold, every other action the jump one.
	void add_source(PadActionBinding& binding, PadControl control, PadAction action)
	{
		const ActionTriggerThreshold threshold = action == PadAction::shoot ?
			ActionTriggerThreshold::shoot : ActionTriggerThreshold::jump;
		switch (control)
		{
		case PadControl::none: return;
		case PadControl::a: binding.buttons.push_back(GamepadButton::a); return;
		case PadControl::b: binding.buttons.push_back(GamepadButton::b); return;
		case PadControl::x: binding.buttons.push_back(GamepadButton::x); return;
		case PadControl::y: binding.buttons.push_back(GamepadButton::y); return;
		case PadControl::left_shoulder: binding.buttons.push_back(GamepadButton::left_shoulder); return;
		case PadControl::right_shoulder: binding.buttons.push_back(GamepadButton::right_shoulder); return;
		case PadControl::left_stick: binding.buttons.push_back(GamepadButton::left_stick); return;
		case PadControl::right_stick: binding.buttons.push_back(GamepadButton::right_stick); return;
		case PadControl::start: binding.buttons.push_back(GamepadButton::start); return;
		case PadControl::left_trigger: binding.triggers.push_back({ GamepadTrigger::left, threshold }); return;
		case PadControl::right_trigger: binding.triggers.push_back({ GamepadTrigger::right, threshold }); return;
		}
		throw std::invalid_argument("Unknown pad control");
	}

	// Reads ["primary"] or ["primary", "alternate"].
	PadActionChoice read_choice(const JsonValue& entry)
	{
		if (entry.size() < 1 || entry.size() > 2) { throw std::invalid_argument("A pad action needs one or two controls"); }
		PadActionChoice choice;
		choice.primary = pad_control_from_name(entry.at(0).as_string());
		if (entry.size() == 2) { choice.alternate = pad_control_from_name(entry.at(1).as_string()); }
		return choice;
	}

	constexpr std::array<PadControl, 4> legacy_faces{ PadControl::a, PadControl::b, PadControl::x, PadControl::y };

	struct MouseName
	{
		MouseButton button;
		std::string_view name;
		std::wstring_view label;
	};
	constexpr std::array<MouseName, 5> mouse_names{ {
		{ MouseButton::left, "mouse_left", L"Left click" }, { MouseButton::right, "mouse_right", L"Right click" },
		{ MouseButton::middle, "mouse_middle", L"Middle click" }, { MouseButton::x1, "mouse_x1", L"Mouse 4" },
		{ MouseButton::x2, "mouse_x2", L"Mouse 5" }
	} };

	constexpr std::array<const char*, keyboard_action_count> keyboard_action_names{
		"move_left", "move_right", "jump", "drop", "shoot", "pause" };

	const MouseName* mouse_entry(MouseButton button)
	{
		for (const auto& value : mouse_names) { if (value.button == button) { return &value; } }
		return nullptr;
	}

	bool unbound(const DesktopControl& control)
	{
		return control.key == Key::none && control.button == MouseButton::none;
	}

	void require_single(const DesktopControl& control)
	{
		if (control.key != Key::none && control.button != MouseButton::none)
		{
			throw std::invalid_argument("A desktop control is a key or a mouse button, not both");
		}
	}
}

bool keyboard_jump_key_supported(Key key)
{
	return std::any_of(key_names.begin(), key_names.end(), [key](const KeyName& value) { return value.key == key; });
}

std::array<Key, 27> keyboard_jump_keys()
{
	std::array<Key, 27> result{};
	for (size_t index = 0; index < key_names.size(); ++index) { result[index] = key_names[index].key; }
	return result;
}

std::optional<GameAction> keyboard_jump_conflict(Key key)
{
	const std::optional<KeyboardAction> other = keyboard_control_conflict(InputBindings{},
		KeyboardAction::jump, DesktopControl{ key, MouseButton::none });
	if (other) { return keyboard_game_action(*other); }
	return std::nullopt;
}

std::array<MouseButton, 5> shoot_mouse_buttons()
{
	std::array<MouseButton, 5> result{};
	for (std::size_t index = 0; index < result.size(); ++index) { result[index] = mouse_names[index].button; }
	return result;
}

bool keyboard_control_supported(KeyboardAction action, const DesktopControl& control)
{
	if (static_cast<std::size_t>(action) >= keyboard_action_count) { return false; }
	if (control.key != Key::none && control.button != MouseButton::none) { return false; }
	if (unbound(control)) { return action == KeyboardAction::pause; }
	if (control.button != MouseButton::none)
	{
		return action == KeyboardAction::shoot && mouse_entry(control.button) != nullptr;
	}
	return keyboard_jump_key_supported(control.key);
}

std::string_view desktop_control_name(const DesktopControl& control)
{
	require_single(control);
	if (unbound(control)) { return "none"; }
	if (control.button != MouseButton::none)
	{
		const MouseName* entry = mouse_entry(control.button);
		if (entry == nullptr) { throw std::invalid_argument("Unsupported mouse button"); }
		return entry->name;
	}
	return input_key_name(control.key);
}

std::wstring_view desktop_control_label(const DesktopControl& control)
{
	require_single(control);
	if (unbound(control)) { return L"None"; }
	if (control.button != MouseButton::none)
	{
		const MouseName* entry = mouse_entry(control.button);
		if (entry == nullptr) { throw std::invalid_argument("Unsupported mouse button"); }
		return entry->label;
	}
	return input_key_label(control.key);
}

DesktopControl desktop_control_from_name(std::string_view name)
{
	if (name == "none") { return {}; }
	for (const auto& value : mouse_names) { if (value.name == name) { return { Key::none, value.button }; } }
	return { input_key_from_name(name), MouseButton::none };
}

std::string_view keyboard_action_name(KeyboardAction action)
{
	return keyboard_action_names.at(static_cast<std::size_t>(action));
}

GameAction keyboard_game_action(KeyboardAction action)
{
	switch (action)
	{
	case KeyboardAction::move_left: return GameAction::move_left;
	case KeyboardAction::move_right: return GameAction::move_right;
	case KeyboardAction::jump: return GameAction::jump;
	case KeyboardAction::drop: return GameAction::drop;
	case KeyboardAction::shoot: return GameAction::shoot;
	case KeyboardAction::pause: return GameAction::pause;
	default: throw std::out_of_range("Unknown keyboard action");
	}
}

DesktopControl keyboard_control(const InputBindings& bindings, KeyboardAction action)
{
	switch (action)
	{
	case KeyboardAction::move_left: return { bindings.keyboard_move_left, MouseButton::none };
	case KeyboardAction::move_right: return { bindings.keyboard_move_right, MouseButton::none };
	case KeyboardAction::jump: return { bindings.keyboard_jump, MouseButton::none };
	case KeyboardAction::drop: return { bindings.keyboard_drop, MouseButton::none };
	case KeyboardAction::shoot: return bindings.keyboard_shoot;
	case KeyboardAction::pause: return { bindings.keyboard_pause, MouseButton::none };
	default: throw std::out_of_range("Unknown keyboard action");
	}
}

void set_keyboard_control(InputBindings& bindings, KeyboardAction action, const DesktopControl& control)
{
	if (!keyboard_control_supported(action, control))
	{
		throw std::invalid_argument("Control is outside keyboard action " +
			std::string(keyboard_action_name(action)) + "'s domain");
	}
	switch (action)
	{
	case KeyboardAction::move_left: bindings.keyboard_move_left = control.key; return;
	case KeyboardAction::move_right: bindings.keyboard_move_right = control.key; return;
	case KeyboardAction::jump: bindings.keyboard_jump = control.key; return;
	case KeyboardAction::drop: bindings.keyboard_drop = control.key; return;
	case KeyboardAction::shoot: bindings.keyboard_shoot = control; return;
	case KeyboardAction::pause: bindings.keyboard_pause = control.key; return;
	default: throw std::out_of_range("Unknown keyboard action");
	}
}

std::optional<KeyboardAction> keyboard_control_conflict(const InputBindings& bindings,
	KeyboardAction action, const DesktopControl& control)
{
	if (unbound(control)) { return std::nullopt; }
	for (std::size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction other = static_cast<KeyboardAction>(index);
		if (other != action && keyboard_control(bindings, other) == control) { return other; }
	}
	return std::nullopt;
}

bool same_keyboard(const InputBindings& left, const InputBindings& right)
{
	for (std::size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		if (keyboard_control(left, action) != keyboard_control(right, action)) { return false; }
	}
	return true;
}

std::string_view input_key_name(Key key)
{
	if (!keyboard_jump_key_supported(key)) { throw std::invalid_argument("Keyboard Jump key must be A-Z or Space"); }
	return labrador::key_name(key);
}

std::wstring_view input_key_label(Key key)
{
	for (const auto& value : key_names) { if (value.key == key) { return value.label; } }
	throw std::invalid_argument("Keyboard Jump key must be A-Z or Space");
}

Key input_key_from_name(std::string_view name)
{
	const std::optional<Key> key = labrador::key_from_name(name);
	if (!key || !keyboard_jump_key_supported(*key)) { throw std::invalid_argument("Unknown keyboard Jump key name"); }
	return *key;
}

std::array<PadControl, 11> pad_controls()
{
	std::array<PadControl, 11> result{};
	for (std::size_t index = 0; index < result.size(); ++index) { result[index] = control_names[index + 1].control; }
	return result;
}

bool pad_control_supported(PadControl control)
{
	return control != PadControl::none && std::any_of(control_names.begin(), control_names.end(),
		[control](const ControlName& value) { return value.control == control; });
}

std::string_view pad_control_name(PadControl control) { return control_entry(control).name; }

std::wstring_view pad_control_label(PadControl control) { return control_entry(control).label; }

PadControl pad_control_from_name(std::string_view name)
{
	for (const auto& value : control_names) { if (value.name == name) { return value.control; } }
	throw std::invalid_argument("Unknown pad control name");
}

std::string_view pad_action_name(PadAction action)
{
	return action_names.at(static_cast<std::size_t>(action));
}

std::optional<PadAction> pad_control_conflict(const PadProfile& profile,
	PadAction action, bool alternate, PadControl control)
{
	if (control == PadControl::none) { return std::nullopt; }
	for (std::size_t index = 0; index < pad_action_count; ++index)
	{
		const PadAction other = static_cast<PadAction>(index);
		const PadActionChoice& choice = profile.actions[index];
		if ((other != action || alternate) && choice.primary == control) { return other; }
		if ((other != action || !alternate) && choice.alternate == control) { return other; }
	}
	return std::nullopt;
}

void PadProfile::validate() const
{
	for (std::size_t index = 0; index < pad_action_count; ++index)
	{
		const PadActionChoice& choice = this->actions[index];
		if (!pad_control_supported(choice.primary))
		{
			throw std::invalid_argument("Pad " + std::string(action_names[index]) + " needs a supported primary control");
		}
		if (choice.alternate != PadControl::none && !pad_control_supported(choice.alternate))
		{
			throw std::invalid_argument("Pad " + std::string(action_names[index]) + " has an unsupported alternate control");
		}
		for (const bool alternate : { false, true })
		{
			const PadControl control = alternate ? choice.alternate : choice.primary;
			if (pad_control_conflict(*this, static_cast<PadAction>(index), alternate, control))
			{
				throw std::invalid_argument("Pad control " + std::string(pad_control_name(control)) +
					" is bound to more than one pad action");
			}
		}
	}
}

void InputBindings::validate_keyboard() const
{
	if (!keyboard_jump_key_supported(this->keyboard_jump))
	{
		throw std::invalid_argument("Keyboard Jump key must be A-Z or Space");
	}
	for (std::size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		const DesktopControl control = keyboard_control(*this, action);
		if (!keyboard_control_supported(action, control))
		{
			throw std::invalid_argument("Keyboard " + std::string(keyboard_action_name(action)) +
				" needs a supported control");
		}
		if (keyboard_control_conflict(*this, action, control))
		{
			throw std::invalid_argument("Keyboard " + std::string(keyboard_action_name(action)) + " control " +
				std::string(desktop_control_name(control)) + " conflicts with another gameplay action");
		}
	}
}

void InputBindings::validate() const
{
	this->validate_keyboard();
	for (const PadProfile& profile : this->pads) { profile.validate(); }
}

ActionBindings effective_action_bindings(const InputBindings& preferences)
{
	preferences.validate();
	ActionBindings result = default_action_bindings();
	const auto desktop = [&result](GameAction action) -> DesktopActionBinding&
	{
		return result.game_desktop[static_cast<std::size_t>(action)];
	};
	desktop(GameAction::move_left).keys = { preferences.keyboard_move_left };
	desktop(GameAction::move_right).keys = { preferences.keyboard_move_right };
	desktop(GameAction::jump).keys = { preferences.keyboard_jump };
	desktop(GameAction::drop).keys = { preferences.keyboard_drop, player_input_consts::drop_key_alternate };
	DesktopActionBinding& shoot = desktop(GameAction::shoot);
	shoot = {};
	if (preferences.keyboard_shoot.button != MouseButton::none) { shoot.mouse_buttons = { preferences.keyboard_shoot.button }; }
	else { shoot.keys = { preferences.keyboard_shoot.key }; }
	desktop(GameAction::pause).keys = { player_input_consts::pause_key };
	if (preferences.keyboard_pause != Key::none) { desktop(GameAction::pause).keys.push_back(preferences.keyboard_pause); }
	return result;
}

ActionBindings effective_action_bindings(const InputBindings& preferences, int pad_slot)
{
	ActionBindings result = effective_action_bindings(preferences);
	const PadProfile& profile = preferences.pads.at(static_cast<std::size_t>(pad_slot));
	const std::array<GameAction, pad_action_count> actions{ GameAction::jump, GameAction::shoot, GameAction::pause };
	for (std::size_t index = 0; index < pad_action_count; ++index)
	{
		PadActionBinding binding;
		add_source(binding, profile.actions[index].primary, static_cast<PadAction>(index));
		add_source(binding, profile.actions[index].alternate, static_cast<PadAction>(index));
		result.game_pad[static_cast<std::size_t>(actions[index])] = std::move(binding);
	}
	return result;
}

PadProfile read_pad_profile(const JsonValue& entry)
{
	PadProfile profile;
	for (std::size_t action = 0; action < pad_action_count; ++action)
	{
		profile.actions[action] = read_choice(entry.array(action_names[action]));
	}
	profile.validate();
	return profile;
}

void write_pad_profile(SaveWriter& writer, const PadProfile& profile)
{
	writer.begin_object();
	for (std::size_t action = 0; action < pad_action_count; ++action)
	{
		const PadActionChoice& choice = profile.actions[action];
		writer.key(action_names[action]);
		writer.begin_array();
		writer.string(std::string(pad_control_name(choice.primary)));
		if (choice.alternate != PadControl::none) { writer.string(std::string(pad_control_name(choice.alternate))); }
		writer.end_array();
	}
	writer.end_object();
}

void InputBindingsSection::reset(SaveData& data) const
{
	data.input_bindings = InputBindings();
}

void InputBindingsSection::read(const JsonValue& root, SaveData& data) const
{
	if (!root.has(this->key())) { return; }
	const JsonValue section = root.object(this->key());
	InputBindings candidate;
	try
	{
		candidate.keyboard_jump = input_key_from_name(section.string("keyboard_jump"));
	}
	catch (const std::runtime_error&) {} // Jump alone recovers to Space.
	catch (const std::invalid_argument&) {}
	if (section.has("keyboard"))
	{
		// The other five recover together: one damaged entry discards them all.
		InputBindings keys = candidate;
		try
		{
			const JsonValue keyboard = section.object("keyboard");
			for (std::size_t index = 0; index < keyboard_action_count; ++index)
			{
				const KeyboardAction action = static_cast<KeyboardAction>(index);
				if (action == KeyboardAction::jump) { continue; }
				set_keyboard_control(keys, action, desktop_control_from_name(
					keyboard.string(keyboard_action_names[index])));
			}
			candidate = keys;
		}
		catch (const std::runtime_error&) {}
		catch (const std::invalid_argument&) {}
	}
	// A conflicting set recovers the five to the defaults first, then Jump to
	// Space if it still clashes with a default.
	const auto keyboard_valid = [](const InputBindings& value)
	{
		try { value.validate_keyboard(); return true; }
		catch (const std::invalid_argument&) { return false; }
	};
	if (!keyboard_valid(candidate))
	{
		const Key jump = candidate.keyboard_jump;
		candidate = InputBindings{};
		candidate.keyboard_jump = jump;
		if (!keyboard_valid(candidate)) { candidate.keyboard_jump = Key::space; }
	}
	if (section.has("pads"))
	{
		try
		{
			const JsonValue slots = section.array("pads");
			for (std::size_t slot = 0; slot < candidate.pads.size() && slot < slots.size(); ++slot)
			{
				try { candidate.pads[slot] = read_pad_profile(slots.at(slot)); }
				catch (const std::runtime_error&) {} // Recover just this slot to the defaults.
				catch (const std::invalid_argument&) {}
			}
		}
		catch (const std::runtime_error&) {} // A malformed array does not discard the keyboard key.
	}
	else if (section.has("pad_jump"))
	{
		// The earlier per-slot face-button Jump choice; LT and the rest keep their defaults.
		try
		{
			const JsonValue slots = section.array("pad_jump");
			for (std::size_t slot = 0; slot < candidate.pads.size() && slot < slots.size(); ++slot)
			{
				try
				{
					const PadControl face = pad_control_from_name(slots.at(slot).as_string());
					if (std::find(legacy_faces.begin(), legacy_faces.end(), face) != legacy_faces.end())
					{
						candidate.pads[slot][PadAction::jump].primary = face;
					}
				}
				catch (const std::runtime_error&) {} // Recover just this slot to A.
				catch (const std::invalid_argument&) {}
			}
		}
		catch (const std::runtime_error&) {}
	}
	data.input_bindings = candidate;
}

void InputBindingsSection::write(SaveWriter& writer, const SaveData& data) const
{
	writer.key(this->key());
	writer.begin_object();
	writer.key("keyboard_jump");
	writer.string(std::string(input_key_name(data.input_bindings.keyboard_jump)));
	writer.key("keyboard");
	writer.begin_object();
	for (std::size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		if (action == KeyboardAction::jump) { continue; }
		writer.key(keyboard_action_names[index]);
		writer.string(std::string(desktop_control_name(keyboard_control(data.input_bindings, action))));
	}
	writer.end_object();
	writer.key("pads");
	writer.begin_array();
	for (const PadProfile& profile : data.input_bindings.pads) { write_pad_profile(writer, profile); }
	writer.end_array();
	writer.end_object();
}
