#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_binds.h"
#include "game/debug/debug_tokenizer.h"

using labrador::Key;

const DebugBinds& DebugBinds::defaults()
{
	static const DebugBinds table = []
		{
			DebugBinds binds;
			binds.bind({ Key::f2 }, "overlay collision");
			binds.bind({ Key::f3 }, "overlay camera");
			binds.bind({ Key::f4 }, "overlay paint");
			binds.bind({ Key::f5 }, "restart");
			binds.bind({ Key::f6 }, "pause toggle");
			binds.bind({ Key::f7 }, "step 1");
			binds.bind({ Key::f9 }, "feedback toggle");
			return binds;
		}();
	return table;
}

void DebugBinds::bind(const KeyChord& chord, std::string line)
{
	this->table_[chord] = std::move(line);
}

bool DebugBinds::unbind(const KeyChord& chord)
{
	return this->table_.erase(chord) != 0;
}

void DebugBinds::clear()
{
	this->table_.clear();
}

void DebugBinds::reset()
{
	this->table_ = defaults().table_;
}

const std::string* DebugBinds::find(const KeyChord& chord) const
{
	const auto found = this->table_.find(chord);
	return found == this->table_.end() ? nullptr : &found->second;
}

std::vector<std::pair<KeyChord, std::string>> DebugBinds::fired(
	const std::function<bool(Key)>& pressed, const std::function<bool(Key)>& held) const
{
	std::vector<std::pair<KeyChord, std::string>> result;
	if (this->table_.empty())
	{
		return result;
	}
	const bool ctrl = held(Key::control);
	const bool shift = held(Key::shift);
	for (const auto& [chord, line] : this->table_)
	{
		if (chord.ctrl == ctrl && chord.shift == shift && pressed(chord.key))
		{
			result.push_back({ chord, line });
		}
	}
	return result;
}

namespace
{
	// A bound line as binds.cfg holds it. The file is read as a script, so
	// '$' is written '$$' (a script substitutes $1-$9 and $$, quoted or not)
	// and a line with '//' is always quoted (an unquoted // starts a comment).
	std::string saved_line(const std::string& line)
	{
		std::string escaped;
		for (const char c : line)
		{
			escaped += c;
			if (c == '$')
			{
				escaped += '$';
			}
		}
		std::string quoted = quote_word(escaped);
		if (quoted.front() != '"' && escaped.find("//") != std::string::npos)
		{
			// quote_word left it bare, so it holds no '"' to escape.
			quoted = '"' + escaped + '"';
		}
		return quoted;
	}
}

std::string DebugBinds::save_text() const
{
	std::string text =
		"// ColourWars debug binds - written by the game (bind, unbind, binds.reset).\n"
		"// Only the differences from the default binds; this file is exec-able.\n";
	for (const auto& [chord, line] : defaults().table_)
	{
		if (this->table_.count(chord) == 0)
		{
			text += "unbind " + debug_keys::chord_name(chord) + "\n";
		}
	}
	for (const auto& [chord, line] : this->table_)
	{
		const std::string* old = defaults().find(chord);
		if (old == nullptr || *old != line)
		{
			text += "bind " + debug_keys::chord_name(chord) + " " + saved_line(line) + "\n";
		}
	}
	return text;
}
#endif
