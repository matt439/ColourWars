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
	struct KeyName
	{
		Key key;
		std::string_view name;
		std::wstring_view label;
	};

	constexpr std::array<KeyName, 27> key_names{ {
		{ Key::a, "a", L"A" }, { Key::b, "b", L"B" }, { Key::c, "c", L"C" },
		{ Key::d, "d", L"D" }, { Key::e, "e", L"E" }, { Key::f, "f", L"F" },
		{ Key::g, "g", L"G" }, { Key::h, "h", L"H" }, { Key::i, "i", L"I" },
		{ Key::j, "j", L"J" }, { Key::k, "k", L"K" }, { Key::l, "l", L"L" },
		{ Key::m, "m", L"M" }, { Key::n, "n", L"N" }, { Key::o, "o", L"O" },
		{ Key::p, "p", L"P" }, { Key::q, "q", L"Q" }, { Key::r, "r", L"R" },
		{ Key::s, "s", L"S" }, { Key::t, "t", L"T" }, { Key::u, "u", L"U" },
		{ Key::v, "v", L"V" }, { Key::w, "w", L"W" }, { Key::x, "x", L"X" },
		{ Key::y, "y", L"Y" }, { Key::z, "z", L"Z" }, { Key::space, "space", L"Space" }
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
	for (const auto& value : key_names) { if (value.key == key) { return value.name; } }
	throw std::invalid_argument("Keyboard Jump key must be A-Z or Space");
}

std::wstring_view input_key_label(Key key)
{
	for (const auto& value : key_names) { if (value.key == key) { return value.label; } }
	throw std::invalid_argument("Keyboard Jump key must be A-Z or Space");
}

Key input_key_from_name(std::string_view name)
{
	for (const auto& value : key_names) { if (value.name == name) { return value.key; } }
	throw std::invalid_argument("Unknown keyboard Jump key name");
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
}

ActionBindings effective_action_bindings(const InputBindings& preferences)
{
	preferences.validate();
	ActionBindings result = default_action_bindings();
	result.game_desktop[static_cast<std::size_t>(GameAction::jump)].keys = { preferences.keyboard_jump };
	return result;
}

void InputBindingsSection::reset(SaveData& data) const
{
	data.input_bindings = InputBindings();
}

void InputBindingsSection::read(const JsonValue& root, SaveData& data) const
{
	if (!root.has(this->key())) { return; }
	InputBindings candidate;
	try
	{
		candidate.keyboard_jump = input_key_from_name(root.object(this->key()).string("keyboard_jump"));
		candidate.validate();
	}
	catch (const std::invalid_argument& error)
	{
		// Save's section boundary catches runtime_error. Keep validation failure
		// local instead of letting it reset every independent preference.
		throw std::runtime_error(error.what());
	}
	data.input_bindings = candidate;
}

void InputBindingsSection::write(SaveWriter& writer, const SaveData& data) const
{
	writer.key(this->key());
	writer.begin_object();
	writer.key("keyboard_jump");
	writer.string(std::string(input_key_name(data.input_bindings.keyboard_jump)));
	writer.end_object();
}
