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

// A named thing the console can reach: a variable or a command.
class DebugEntry
{
public:
	virtual ~DebugEntry() = default;

	DebugEntry(const DebugEntry&) = delete;
	DebugEntry& operator=(const DebugEntry&) = delete;

	// [a-z0-9_]+, unique across variables and commands.
	const std::string& name() const;

	// One line, shown by help and list.
	const std::string& help() const;

	virtual bool is_variable() const = 0;

	// "name <args>" for help and wrong-count errors.
	virtual std::string usage() const = 0;

	// Completion for the word at index `argument` (0 = first argument).
	virtual std::vector<std::string> suggestions(std::size_t argument) const = 0;

protected:
	DebugEntry(std::string name, std::string help);

private:
	std::string name_;
	std::string help_;
};
