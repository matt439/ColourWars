#pragma once

#include <cstddef>
#include <string>
#include <vector>

class DebugDirectorImpl;
class DebugEntry;

// One row of the debug menu as the page draws it.
struct DebugMenuRow
{
	enum class Kind
	{
		apply,    // close the menu so what is staged applies
		discard,  // drop what is staged
		category, // open a category
		variable, // name: value; Left/Right step it
		run,      // run the command
		pick,     // open a picker over the command's choices
		choice,   // one picker entry: runs `line`
		note,     // text only
		more,     // the next page of a long tuning node
	};

	Kind kind = Kind::note;
	std::string label;
	std::string value;
	std::string line;
	bool enabled = true;
};

// The debug menu without its widgets: categories from the registry, their
// entries, and pickers over a command's or a choice variable's words. Every
// action runs as a console line through the director (echoed "[menu] ..."),
// so the log and transcript say what the menu did. Pure apart from that, so
// the acceptance case drives it without a window.
//
// Staging: entries marked staged (stage, players, weapon, visual) only queue
// into the director's match request, so the menu stays open and several can
// be combined; the Apply row closes the menu and the request applies through
// the normal path. Any other statement that asks the console to close (restart,
// step, screenshot, goto, respawn, menu, quit) closes the menu.
class DebugMenuModel
{
public:
	enum class Result
	{
		stay,
		close,
	};

	// The page lays out at most this many rows; longer pickers end with a note.
	static constexpr std::size_t max_rows = 12;

	explicit DebugMenuModel(DebugDirectorImpl& director);

	// Rebuilt from live values on every call.
	std::vector<DebugMenuRow> rows() const;

	// "DEBUG", "DEBUG / view", "DEBUG / match / stage".
	std::string title() const;

	// "Queued: stage x, players 2" or empty.
	std::string queued() const;

	Result confirm(std::size_t row);
	Result step(std::size_t row, int direction);

	// Picker -> entries -> categories -> close.
	Result back();

	// Changes whenever the level (categories, entries, a picker) changes, so
	// the page rebuilds and focuses focus_hint().
	int level() const { return this->level_; }
	std::size_t focus_hint() const { return this->focus_hint_; }

private:
	DebugDirectorImpl& director_;
	std::string category_;
	std::string picker_;

	// The tuning category walks the content files' paths: the node shown
	// ("" for the two files, else a prefix ending in '.' or '['), and which
	// page of it. Left/Right on a leaf runs `tune <path> next|prev|toggle`.
	std::string tune_node_;
	std::size_t tune_page_ = 0;
	std::vector<DebugMenuRow> tuning_rows() const;
	std::size_t focus_hint_ = 0;
	int level_ = 0;

	std::vector<DebugMenuRow> collect() const;
	const DebugEntry* entry(const std::string& name) const;
	Result run(const std::string& line, bool staged);
	void enter(std::string category, std::string picker);
	void enter_tune_node(std::string node, const std::string& focus_line);
	std::size_t row_of(const std::string& label) const;
};
