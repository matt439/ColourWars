#pragma once

#include <cstddef>
#include <string>
#include <vector>

// One frame of editing input, already reduced from the keyboard so the field
// can be driven without a window. Each flag is one (possibly repeated) press.
struct DebugKeyEvents
{
	std::string typed;
	bool left = false;
	bool right = false;
	bool home = false;
	bool end = false;
	bool backspace = false;
	bool del = false;
	bool up = false;
	bool down = false;
	bool clear_line = false;
};

// Repeat for keys the engine does not repeat (it repeats typed text only):
// fires on the press, then after `delay` every `interval` while held.
class DebugKeyRepeat
{
public:
	static constexpr float delay = 0.40f;
	static constexpr float interval = 1.0f / 30.0f;

	bool fire(bool held, bool pressed, float dt);

private:
	float held_time_ = 0.0f;
	float next_fire_ = 0.0f;
};

// A single-line editor with a caret and an in-memory history.
class DebugTextField
{
public:
	static constexpr std::size_t max_length = 200;
	static constexpr std::size_t history_capacity = 64;

	void apply(const DebugKeyEvents& events);

	const std::string& text() const;
	std::size_t caret() const;

	// Replaces the line (Tab completion); the caret moves to the end.
	void set_text(std::string text);

	// Returns the line and clears it. A non-blank line is added to history
	// unless it repeats the most recent entry.
	std::string submit();

	const std::vector<std::string>& history() const;

	// Replaces the history (history.txt, read at startup): the last
	// history_capacity entries that are not blank and fit max_length, with
	// adjacent repeats collapsed; browsing starts again from the draft.
	void set_history(const std::vector<std::string>& lines);

private:
	std::string text_;
	std::size_t caret_ = 0;
	std::vector<std::string> history_;

	// -1 edits the draft; otherwise an index into history_.
	int history_index_ = -1;
	std::string draft_;

	void browse(int direction);
};
