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

	PadProfile read_profile(const JsonValue& entry)
	{
		PadProfile profile;
		for (std::size_t action = 0; action < pad_action_count; ++action)
		{
			profile.actions[action] = read_choice(entry.array(action_names[action]));
		}
		profile.validate();
		return profile;
	}

	constexpr std::array<PadControl, 4> legacy_faces{ PadControl::a, PadControl::b, PadControl::x, PadControl::y };
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
	const auto& desktop = default_action_bindings().game_desktop;
	for (std::size_t index = 0; index < desktop.size(); ++index)
	{
		const GameAction action = static_cast<GameAction>(index);
		if (action == GameAction::jump || action == GameAction::skip_intro) { continue; }
		const auto& keys = desktop[index].keys;
		if (std::find(keys.begin(), keys.end(), key) != keys.end()) { return action; }
	}
	return std::nullopt;
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

void InputBindings::validate() const
{
	if (!keyboard_jump_key_supported(this->keyboard_jump))
	{
		throw std::invalid_argument("Keyboard Jump key must be A-Z or Space");
	}
	if (keyboard_jump_conflict(this->keyboard_jump))
	{
		throw std::invalid_argument("Keyboard Jump key " + std::string(input_key_name(this->keyboard_jump)) +
			" conflicts with another gameplay action");
	}
	for (const PadProfile& profile : this->pads) { profile.validate(); }
}

ActionBindings effective_action_bindings(const InputBindings& preferences)
{
	preferences.validate();
	ActionBindings result = default_action_bindings();
	result.game_desktop[static_cast<std::size_t>(GameAction::jump)].keys = { preferences.keyboard_jump };
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
		candidate.validate();
	}
	catch (const std::runtime_error&)
	{
		candidate.keyboard_jump = Key::space;
	}
	catch (const std::invalid_argument&)
	{
		candidate.keyboard_jump = Key::space;
	}
	if (section.has("pads"))
	{
		try
		{
			const JsonValue slots = section.array("pads");
			for (std::size_t slot = 0; slot < candidate.pads.size() && slot < slots.size(); ++slot)
			{
				try { candidate.pads[slot] = read_profile(slots.at(slot)); }
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
	writer.key("pads");
	writer.begin_array();
	for (const PadProfile& profile : data.input_bindings.pads)
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
	writer.end_array();
	writer.end_object();
}
