#pragma once

#include "game/debug/debug_command.h"
#include "game/debug/debug_variable.h"
#include <map>
#include <memory>

class DebugLog;

// The result of Tab: the edited line, and the candidates when ambiguous.
struct DebugCompletion
{
	std::string line;
	std::vector<std::string> candidates;
};

// Named variables and commands, and the built-ins that reach them
// (set, get, list, help). The console and launch arguments drive it.
class DebugRegistry
{
public:
	DebugRegistry();
	virtual ~DebugRegistry() = default;

	DebugRegistry(const DebugRegistry&) = delete;
	DebugRegistry& operator=(const DebugRegistry&) = delete;

	// A duplicate name is a programming error: std::logic_error.
	void add(std::unique_ptr<DebugEntry> entry);

	DebugEntry* find(std::string_view name) const;
	DebugVariable* variable(std::string_view name) const;

	// Sorted by name.
	std::vector<const DebugEntry*> entries() const;

	DebugCompletion complete(std::string_view line) const;

	// Runs every ';'-separated statement. Exceptions thrown by a variable or
	// command are caught and logged; none escapes. Returns the statuses OR-ed:
	// ok only if every statement succeeded, close_console if any asked.
	DebugStatus execute(std::string_view line, DebugLog& log);

private:
	std::map<std::string, std::unique_ptr<DebugEntry>, std::less<>> entries_;

	DebugStatus execute_statement(const std::string& statement, DebugLog& log);
	void add_builtins();
};
