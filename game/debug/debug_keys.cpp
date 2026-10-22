#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_keys.h"
#include "game/objects/player_input.h"
#include <cctype>

using labrador::Key;

namespace
{
	Key offset(Key first, int index)
	{
		return static_cast<Key>(static_cast<int>(first) + index);
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
				for (int i = 0; i < 26; ++i)
				{
					result.push_back({ offset(Key::a, i), std::string(1, static_cast<char>('a' + i)) });
				}
				for (int i = 0; i < 10; ++i)
				{
					result.push_back({ offset(Key::digit_0, i), std::string(1, static_cast<char>('0' + i)) });
				}
				for (int i = 0; i < 12; ++i)
				{
					result.push_back({ offset(Key::f1, i), "f" + std::to_string(i + 1) });
				}
				for (int i = 0; i < 10; ++i)
				{
					result.push_back({ offset(Key::numpad_0, i), "numpad" + std::to_string(i) });
				}
				const std::pair<Key, const char*> named[] = {
					{ Key::space, "space" }, { Key::tab, "tab" }, { Key::enter, "enter" },
					{ Key::backspace, "backspace" }, { Key::insert, "insert" },
					{ Key::del, "delete" }, { Key::home, "home" }, { Key::end, "end" },
					{ Key::page_up, "pageup" }, { Key::page_down, "pagedown" },
					{ Key::left, "left" }, { Key::right, "right" }, { Key::up, "up" },
					{ Key::down, "down" }, { Key::minus, "minus" }, { Key::equals, "equals" },
					{ Key::left_bracket, "lbracket" }, { Key::right_bracket, "rbracket" },
					{ Key::backslash, "backslash" }, { Key::semicolon, "semicolon" },
					{ Key::apostrophe, "apostrophe" }, { Key::comma, "comma" },
					{ Key::period, "period" }, { Key::slash, "slash" }, { Key::pause, "pause" },
					{ Key::escape, "escape" }, { Key::grave, "grave" },
				};
				for (const auto& [key, name] : named)
				{
					result.push_back({ key, name });
				}
				return result;
			}();
		return table;
	}

	std::string key_name(Key key)
	{
		for (const auto& [candidate, name] : names())
		{
			if (candidate == key)
			{
				return name;
			}
		}
		return {};
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
		for (const auto& [key, name] : names())
		{
			if (name == rest)
			{
				chord.key = key;
				return chord;
			}
		}
		return std::nullopt;
	}

	std::string chord_name(const KeyChord& chord)
	{
		return std::string(chord.ctrl ? "ctrl+" : "") + (chord.shift ? "shift+" : "") +
			debug_keys::key_name(chord.key);
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
