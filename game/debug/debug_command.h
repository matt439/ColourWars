#pragma once

#include "game/debug/debug_entry.h"
#include <functional>

class DebugLog;

// One argument of a command: its name in usage text, whether it may be left
// out, and (for completion) the words it suggests.
struct DebugArgSpec
{
	std::string name;
	bool optional = false;
	std::function<std::vector<std::string>()> choices;
};

// The words after the command name. Parsing helpers report bad input through
// their `ok` flag rather than throwing, so a command can name the argument.
class DebugArgs
{
public:
	explicit DebugArgs(std::vector<std::string> words);

	std::size_t size() const;
	bool has(std::size_t index) const;
	const std::string& text(std::size_t index) const;

	// Whole numbers and decimals only; trailing junk is not a number.
	static bool parse_int(std::string_view text, int& value);
	static bool parse_float(std::string_view text, float& value);

private:
	std::vector<std::string> words_;
};

// A console verb with typed argument specs. The registry checks the argument
// count against args() before run() is called.
class DebugCommand : public DebugEntry
{
public:
	bool is_variable() const override { return false; }
	std::string usage() const override;
	std::vector<std::string> suggestions(std::size_t argument) const override;

	const std::vector<DebugArgSpec>& args() const;

	// Extra words beyond args() are accepted (for free text).
	bool variadic() const;

	virtual DebugStatus run(const DebugArgs& args, DebugLog& log) = 0;

protected:
	DebugCommand(std::string name, std::string help,
		std::vector<DebugArgSpec> args, bool variadic = false);

private:
	std::vector<DebugArgSpec> args_;
	bool variadic_ = false;
};

// A command whose body is a callable: what every phase-1 command is.
class LambdaCommand final : public DebugCommand
{
public:
	using Body = std::function<DebugStatus(const DebugArgs&, DebugLog&)>;

	LambdaCommand(std::string name, std::string help,
		std::vector<DebugArgSpec> args, Body body, bool variadic = false);

	DebugStatus run(const DebugArgs& args, DebugLog& log) override;

private:
	Body body_;
};
