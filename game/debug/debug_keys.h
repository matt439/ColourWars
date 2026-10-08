#pragma once

#include "engine/input/keyboard.h"
#include <compare>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// A key with the modifiers a debug bind needs held: "f5", "shift+f5",
// "ctrl+shift+f2". Ctrl and Shift only: Alt+F4, Alt+Space and Alt+Tab belong
// to the system whatever a bind says. The engine names no keys (engine report
// 13), so the table is here. Debug-only bodies.
struct KeyChord
{
	labrador::Key key = labrador::Key::none;
	bool ctrl = false;
	bool shift = false;

	auto operator<=>(const KeyChord&) const = default;
};

namespace debug_keys
{
	// Every key a bind may name, in a stable order: a-z, 0-9, f1-f12,
	// numpad0-9, then the named keys. Lower case.
	const std::vector<std::pair<labrador::Key, std::string>>& names();

	// The table's name for a key, or empty.
	std::string key_name(labrador::Key key);

	// Case-insensitive; "ctrl+" and "shift+" in any order before the key.
	std::optional<KeyChord> parse(std::string_view text);

	// "shift+f5": the form parse reads back.
	std::string chord_name(const KeyChord& chord);

	// grave (the console), f10 and f11 (the debug menu), with any modifier.
	bool reserved(const KeyChord& chord);

	// A key the keyboard player also reads in a match (A, D, S, Space, Down,
	// Escape, F1), unmodified: a bind on it is allowed with a warning.
	bool player_key(const KeyChord& chord);
}
