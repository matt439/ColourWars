#pragma once

#include "game/debug/commands/debug_command.h"
#include "game/debug/commands/debug_variable.h"
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
	// Listed under `menu` (DebugMenuTraits::of) instead of the entry's own.
	void add(std::unique_ptr<DebugEntry> entry, DebugMenuTraits menu);
	// A LambdaCommand listed under `menu`: how the director registers a
	// command whose body is a callable.
	void add_command(std::string name, std::string help, std::vector<DebugArgSpec> args,
		LambdaCommand::Body body, DebugMenuTraits menu, bool variadic = false);

	// What `help` lists after the console and menu keys: the key binds as they
	// are now. Unset, it lists none.
	void set_help_keys(std::function<std::string()> keys);

	DebugEntry* find(std::string_view name) const;
	DebugVariable* variable(std::string_view name) const;

	// Sorted by name.
	std::vector<const DebugEntry*> entries() const;

	// The debug menu's categories, in the order it lists them.
	static const std::vector<std::string>& category_order();

	// The category_order() entries that have a menu-visible entry.
	std::vector<std::string> categories() const;

	// Menu-visible entries of one category, sorted by name.
	std::vector<const DebugEntry*> entries(std::string_view category) const;

	DebugCompletion complete(std::string_view line) const;

	// Runs every ';'-separated statement. Exceptions thrown by a variable or
	// command are caught and logged; none escapes. Returns the statuses OR-ed:
	// ok only if every statement succeeded, close_console if any asked; the
	// message is the first failure's (already logged), for a script's error.
	DebugStatus execute(std::string_view line, DebugLog& log);

	// Whether one statement names an entry and gives it a word count it
	// accepts, without running it: the shipped scripts' lint.
	DebugStatus check(std::string_view statement) const;

private:
	std::map<std::string, std::unique_ptr<DebugEntry>, std::less<>> entries_;
	std::function<std::string()> help_keys_;

	DebugStatus execute_statement(const std::string& statement, DebugLog& log);
	void add_builtins();
};
