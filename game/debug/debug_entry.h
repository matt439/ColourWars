#pragma once

#include <string>
#include <string_view>
#include <vector>

// What a console statement reports back.
struct DebugStatus
{
	bool ok = true;

	// Printed by the console; empty prints nothing.
	std::string message;

	// Set by commands whose effect applies only once the world updates again
	// (a rebuild, a step): the console closes so the request can apply.
	bool close_console = false;

	static DebugStatus success(std::string message = {}, bool close_console = false);
	static DebugStatus failure(std::string message);
};

// How the debug menu shows an entry. Declared per entry where it is
// registered rather than derived from its arguments, so the menu says exactly
// what each one is.
struct DebugMenuTraits
{
	enum class Form
	{
		hidden,   // console only (set, get, list, help, clear, logfile)
		run,      // confirm runs the bare name
		pick,     // confirm opens a picker over one argument's choices
		variable, // name: value; Left/Right step it
	};

	// One of DebugRegistry::category_order().
	std::string category = "console";
	Form form = Form::hidden;

	// For pick: which argument the picker chooses, and the words placed
	// before it ("all" for `weapon all <name>`).
	std::size_t pick_argument = 0;
	std::string prefix;

	// Queues into the match request instead of acting: the menu stays open
	// so several can be staged and applied together.
	bool staged = false;
};

// A named thing the console can reach: a variable or a command.
class DebugEntry
{
public:
	virtual ~DebugEntry() = default;

	DebugEntry(const DebugEntry&) = delete;
	DebugEntry& operator=(const DebugEntry&) = delete;

	// [a-z0-9_.]+, unique across variables and commands. The registry does not
	// check the spelling; '.' groups related variables (overlay.collision).
	const std::string& name() const;

	// One line, shown by help and list.
	const std::string& help() const;

	virtual bool is_variable() const = 0;

	// "name <args>" for help and wrong-count errors.
	virtual std::string usage() const = 0;

	// Completion for the word at index `argument` (0 = first argument).
	virtual std::vector<std::string> suggestions(std::size_t argument) const = 0;

	// What Tab offers for the word being typed, given the arguments before it
	// and the partial word itself. The default is suggestions(previous.size());
	// `tune` uses the partial word to offer one path segment at a time.
	virtual std::vector<std::string> completions(const std::vector<std::string>& previous,
		std::string_view partial) const;


	const DebugMenuTraits& menu() const;
	void set_menu(DebugMenuTraits traits);

protected:
	DebugEntry(std::string name, std::string help);

private:
	std::string name_;
	std::string help_;
	DebugMenuTraits menu_;
};
