#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_command.h"
#include <charconv>

DebugArgs::DebugArgs(std::vector<std::string> words) : words_(std::move(words))
{
}

std::size_t DebugArgs::size() const
{
	return this->words_.size();
}

bool DebugArgs::has(std::size_t index) const
{
	return index < this->words_.size();
}

const std::string& DebugArgs::text(std::size_t index) const
{
	return this->words_.at(index);
}

bool DebugArgs::parse_int(std::string_view text, int& value)
{
	if (!text.empty() && text.front() == '+')
	{
		text.remove_prefix(1);
	}
	const char* end = text.data() + text.size();
	const auto result = std::from_chars(text.data(), end, value);
	return !text.empty() && result.ec == std::errc() && result.ptr == end;
}

bool DebugArgs::parse_float(std::string_view text, float& value)
{
	if (!text.empty() && text.front() == '+')
	{
		text.remove_prefix(1);
	}
	const char* end = text.data() + text.size();
	const auto result = std::from_chars(text.data(), end, value);
	return !text.empty() && result.ec == std::errc() && result.ptr == end &&
		std::isfinite(value);
}

DebugCommand::DebugCommand(std::string name, std::string help,
	std::vector<DebugArgSpec> args, bool variadic) :
	DebugEntry(std::move(name), std::move(help)), args_(std::move(args)), variadic_(variadic)
{
}

std::string DebugCommand::usage() const
{
	std::string usage = this->name();
	for (const DebugArgSpec& arg : this->args_)
	{
		usage += arg.optional ? " [" + arg.name + "]" : " <" + arg.name + ">";
	}
	if (this->variadic_)
	{
		usage += " ...";
	}
	return usage;
}

std::vector<std::string> DebugCommand::suggestions(std::size_t argument) const
{
	if (argument < this->args_.size() && this->args_[argument].choices)
	{
		return this->args_[argument].choices();
	}
	return {};
}

const std::vector<DebugArgSpec>& DebugCommand::args() const
{
	return this->args_;
}

bool DebugCommand::variadic() const
{
	return this->variadic_;
}

LambdaCommand::LambdaCommand(std::string name, std::string help,
	std::vector<DebugArgSpec> args, Body body, bool variadic) :
	DebugCommand(std::move(name), std::move(help), std::move(args), variadic),
	body_(std::move(body))
{
}

DebugStatus LambdaCommand::run(const DebugArgs& args, DebugLog& log)
{
	return this->body_(args, log);
}
#endif
