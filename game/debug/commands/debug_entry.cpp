#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/commands/debug_entry.h"

DebugStatus DebugStatus::success(std::string message, bool close_console)
{
	DebugStatus status;
	status.message = std::move(message);
	status.close_console = close_console;
	return status;
}

DebugStatus DebugStatus::failure(std::string message)
{
	DebugStatus status;
	status.ok = false;
	status.message = std::move(message);
	return status;
}

DebugMenuTraits DebugMenuTraits::of(std::string category, Form form, bool staged,
	std::size_t pick_argument, std::string prefix)
{
	DebugMenuTraits traits;
	traits.category = std::move(category);
	traits.form = form;
	traits.staged = staged;
	traits.pick_argument = pick_argument;
	traits.prefix = std::move(prefix);
	return traits;
}

DebugEntry::DebugEntry(std::string name, std::string help) :
	name_(std::move(name)), help_(std::move(help))
{
}

const std::string& DebugEntry::name() const
{
	return this->name_;
}

const std::string& DebugEntry::help() const
{
	return this->help_;
}

const DebugMenuTraits& DebugEntry::menu() const
{
	return this->menu_;
}

std::vector<std::string> DebugEntry::completions(const std::vector<std::string>& previous,
	std::string_view) const
{
	return this->suggestions(previous.size());
}

void DebugEntry::set_menu(DebugMenuTraits traits)
{
	this->menu_ = std::move(traits);
}
#endif
