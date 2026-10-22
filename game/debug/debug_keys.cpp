#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_keys.h"
#include "game/objects/player_input.h"
#include <algorithm>
#include <cctype>
#include <iterator>

using labrador::Key;

namespace
{
	// Every key a bind may name, in the order names() lists them. Which keys
	// is policy: Shift and Control are chord modifiers, not keys to bind, and
	// the lock keys, Print Screen and the numpad operators were never offered.
	constexpr Key bindable[] = {
		Key::a, Key::b, Key::c, Key::d, Key::e, Key::f, Key::g, Key::h, Key::i, Key::j, Key::k, Key::l, Key::m,
		Key::n, Key::o, Key::p, Key::q, Key::r, Key::s, Key::t, Key::u, Key::v, Key::w, Key::x, Key::y, Key::z,
		Key::digit_0, Key::digit_1, Key::digit_2, Key::digit_3, Key::digit_4,
		Key::digit_5, Key::digit_6, Key::digit_7, Key::digit_8, Key::digit_9,
		Key::f1, Key::f2, Key::f3, Key::f4, Key::f5, Key::f6, Key::f7, Key::f8, Key::f9, Key::f10, Key::f11, Key::f12,
		Key::numpad_0, Key::numpad_1, Key::numpad_2, Key::numpad_3, Key::numpad_4,
		Key::numpad_5, Key::numpad_6, Key::numpad_7, Key::numpad_8, Key::numpad_9,
		Key::space, Key::tab, Key::enter, Key::backspace, Key::insert, Key::del, Key::home, Key::end,
		Key::page_up, Key::page_down, Key::left, Key::right, Key::up, Key::down, Key::minus, Key::equals,
		Key::left_bracket, Key::right_bracket, Key::backslash, Key::semicolon, Key::apostrophe, Key::comma,
		Key::period, Key::slash, Key::pause, Key::escape, Key::grave,
	};

	// The console's spellings where they predate and differ from the engine's
	// tokens. binds.cfg is written in them, so they stay what chord_name writes;
	// parse also takes the engine's own token for each.
	constexpr std::pair<Key, std::string_view> console_spellings[] = {
		{ Key::digit_0, "0" }, { Key::digit_1, "1" }, { Key::digit_2, "2" }, { Key::digit_3, "3" },
		{ Key::digit_4, "4" }, { Key::digit_5, "5" }, { Key::digit_6, "6" }, { Key::digit_7, "7" },
		{ Key::digit_8, "8" }, { Key::digit_9, "9" },
		{ Key::numpad_0, "numpad0" }, { Key::numpad_1, "numpad1" }, { Key::numpad_2, "numpad2" },
		{ Key::numpad_3, "numpad3" }, { Key::numpad_4, "numpad4" }, { Key::numpad_5, "numpad5" },
		{ Key::numpad_6, "numpad6" }, { Key::numpad_7, "numpad7" }, { Key::numpad_8, "numpad8" },
		{ Key::numpad_9, "numpad9" },
		{ Key::del, "delete" }, { Key::page_up, "pageup" }, { Key::page_down, "pagedown" },
		{ Key::left_bracket, "lbracket" }, { Key::right_bracket, "rbracket" },
	};

	std::string_view spelling(Key key)
	{
		for (const auto& [candidate, name] : console_spellings)
		{
			if (candidate == key)
			{
				return name;
			}
		}
		return labrador::key_name(key);
	}

	std::string lower(std::string_view text)
	{
		std::string result(text);
		for (char& c : result)
		{
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return result;
	}
}

namespace debug_keys
{
	const std::vector<std::pair<Key, std::string>>& names()
	{
		static const std::vector<std::pair<Key, std::string>> table = []
			{
				std::vector<std::pair<Key, std::string>> result;
				for (const Key key : bindable)
				{
					result.push_back({ key, std::string(spelling(key)) });
				}
				return result;
			}();
		return table;
	}

	std::optional<KeyChord> parse(std::string_view text)
	{
		std::string rest = lower(text);
		KeyChord chord;
		for (;;)
		{
			if (rest.rfind("ctrl+", 0) == 0 && !chord.ctrl)
			{
				chord.ctrl = true;
				rest.erase(0, 5);
			}
			else if (rest.rfind("shift+", 0) == 0 && !chord.shift)
			{
				chord.shift = true;
				rest.erase(0, 6);
			}
			else
			{
				break;
			}
		}
		const std::optional<Key> engine_key = labrador::key_from_name(rest);
		for (const auto& [key, name] : names())
		{
			if (name == rest || engine_key == key)
			{
				chord.key = key;
				return chord;
			}
		}
		return std::nullopt;
	}

	std::string chord_name(const KeyChord& chord)
	{
		const bool named = std::find(std::begin(bindable), std::end(bindable), chord.key) != std::end(bindable);
		return std::string(chord.ctrl ? "ctrl+" : "") + (chord.shift ? "shift+" : "") +
			(named ? std::string(spelling(chord.key)) : std::string());
	}

	bool reserved(const KeyChord& chord)
	{
		return chord.key == Key::grave || chord.key == Key::f10 || chord.key == Key::f11;
	}

	bool player_key(const KeyChord& chord)
	{
		using namespace player_input_consts;
		if (chord.ctrl || chord.shift)
		{
			return false;
		}
		for (const Key key : { move_left_key, move_right_key, jump_key, drop_key,
			drop_key_alternate, pause_key, debug_key })
		{
			if (chord.key == key)
			{
				return true;
			}
		}
		return false;
	}
}
#endif
