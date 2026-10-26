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

	struct ButtonName
	{
		GamepadButton button;
		std::string_view name;
		std::wstring_view label;
	};
	constexpr std::array<ButtonName, 4> button_names{ {
		{ GamepadButton::a, "a", L"A" }, { GamepadButton::b, "b", L"B" },
		{ GamepadButton::x, "x", L"X" }, { GamepadButton::y, "y", L"Y" }
	} };
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

bool pad_jump_button_supported(GamepadButton button)
{
	return std::any_of(button_names.begin(), button_names.end(),
		[button](const ButtonName& value) { return value.button == button; });
}

std::string_view input_button_name(GamepadButton button)
{
	for (const auto& value : button_names) { if (value.button == button) { return value.name; } }
	throw std::invalid_argument("Pad Jump button must be A, B, X or Y");
}

std::wstring_view input_button_label(GamepadButton button)
{
	for (const auto& value : button_names) { if (value.button == button) { return value.label; } }
	throw std::invalid_argument("Pad Jump button must be A, B, X or Y");
}

GamepadButton input_button_from_name(std::string_view name)
{
	for (const auto& value : button_names) { if (value.name == name) { return value.button; } }
	throw std::invalid_argument("Unknown pad Jump button name");
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
	for (GamepadButton button : this->pad_jump)
	{
		if (!pad_jump_button_supported(button))
		{
			throw std::invalid_argument("Pad Jump button must be A, B, X or Y");
		}
	}
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
	result.game_pad[static_cast<std::size_t>(GameAction::jump)].buttons =
		{ preferences.pad_jump.at(static_cast<std::size_t>(pad_slot)) };
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
	if (section.has("pad_jump"))
	{
		try
		{
			const JsonValue slots = section.array("pad_jump");
			for (std::size_t slot = 0; slot < candidate.pad_jump.size() && slot < slots.size(); ++slot)
			{
				try { candidate.pad_jump[slot] = input_button_from_name(slots.at(slot).as_string()); }
				catch (const std::runtime_error&) {} // Recover just this slot to A.
				catch (const std::invalid_argument&) {}
			}
		}
		catch (const std::runtime_error&) {} // A malformed array does not discard the keyboard key.
	}
	data.input_bindings = candidate;
}

void InputBindingsSection::write(SaveWriter& writer, const SaveData& data) const
{
	writer.key(this->key());
	writer.begin_object();
	writer.key("keyboard_jump");
	writer.string(std::string(input_key_name(data.input_bindings.keyboard_jump)));
	writer.key("pad_jump");
	writer.begin_array();
	for (GamepadButton button : data.input_bindings.pad_jump)
	{
		writer.string(std::string(input_button_name(button)));
	}
	writer.end_array();
	writer.end_object();
}
