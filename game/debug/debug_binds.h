#pragma once

#include "game/debug/debug_keys.h"
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

// The match's debug key table: a key chord to a console line. Its defaults
// are the F2-F9 keys every phase so far has documented; `bind` and `unbind`
// change it and binds.cfg keeps the differences. Data, not code: which key
// runs which line. Debug-only bodies.
class DebugBinds
{
public:
	using Table = std::map<KeyChord, std::string>;

	// F2 overlay collision, F3 overlay camera, F4 overlay paint, F5 restart,
	// F6 pause toggle, F7 step 1, F8 visual next, F9 feedback toggle.
	static const DebugBinds& defaults();

	void bind(const KeyChord& chord, std::string line);
	// False: nothing was bound there.
	bool unbind(const KeyChord& chord);
	void clear();
	void reset();
	const std::string* find(const KeyChord& chord) const;
	const Table& entries() const { return this->table_; }

	// The binds whose key was pressed this frame with exactly their modifiers
	// held (Shift+F5 does not run plain F5), in table order.
	std::vector<std::pair<KeyChord, std::string>> fired(
		const std::function<bool(labrador::Key)>& pressed,
		const std::function<bool(labrador::Key)>& held) const;

	// binds.cfg: a two-line header, then `unbind` for each default removed and
	// `bind` for each key added or changed, in key order. Exec-able.
	std::string save_text() const;

	bool operator==(const DebugBinds& other) const { return this->table_ == other.table_; }

private:
	Table table_;
};
