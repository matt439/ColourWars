#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_wait.h"

namespace
{
	std::string with_hint(std::string text, const DebugWaitContext& context)
	{
		const std::string hint = debug_wait_hint(context);
		return hint.empty() ? text : text + "; " + hint;
	}
}

std::string debug_wait_hint(const DebugWaitContext& context)
{
	if (!context.window_active)
	{
		return "the window is inactive - click it";
	}
	if (context.in_match && context.paused)
	{
		return "the clock is paused - 'pause off' or 'exec.stop'";
	}
	return {};
}

std::optional<std::string> DebugWait::failed(const DebugWaitContext&) const
{
	return std::nullopt;
}

bool DebugWait::should_stop(const DebugWaitContext&) const
{
	return false;
}

bool UpdatesWait::satisfied(const DebugWaitContext& context) const
{
	return context.updates >= this->target_;
}

std::string UpdatesWait::describe(const DebugWaitContext& context) const
{
	const long long left = std::max(0LL, this->target_ - context.updates);
	return with_hint("wait " + std::to_string(this->count_) + " updates (" +
		std::to_string(left) + " left)", context);
}

std::optional<std::string> UpdatesWait::failed(const DebugWaitContext& context) const
{
	if (!context.in_match && !context.build_pending)
	{
		return std::string("the match ended while waiting");
	}
	// The results page does not advance by itself: no update will come.
	if (context.in_match && context.round_over && !context.build_pending &&
		!this->satisfied(context))
	{
		return "the round ended while waiting (" + std::to_string(this->target_ - context.updates) +
			" updates left)";
	}
	return std::nullopt;
}

bool UpdatesWait::should_stop(const DebugWaitContext& context) const
{
	return this->satisfied(context);
}

bool FramesWait::satisfied(const DebugWaitContext& context) const
{
	return context.frames >= this->target_;
}

std::string FramesWait::describe(const DebugWaitContext& context) const
{
	const long long left = std::max(0LL, this->target_ - context.frames);
	return "wait frames " + std::to_string(this->count_) + " (" + std::to_string(left) + " left)";
}

bool MatchWait::satisfied(const DebugWaitContext& context) const
{
	return context.match_ready;
}

std::string MatchWait::describe(const DebugWaitContext& context) const
{
	return with_hint(context.in_match && !context.build_pending ?
		"wait match (countdown)" : "wait match (building)", context);
}

std::optional<std::string> MatchWait::failed(const DebugWaitContext& context) const
{
	if (!context.in_match && !context.build_pending)
	{
		return std::string("no match is running or queued");
	}
	return std::nullopt;
}

bool MatchWait::should_stop(const DebugWaitContext& context) const
{
	return this->satisfied(context);
}

bool ReplayWait::satisfied(const DebugWaitContext& context) const
{
	return !context.replay_active;
}

std::string ReplayWait::describe(const DebugWaitContext& context) const
{
	return with_hint("wait replay", context);
}

bool RecordWait::satisfied(const DebugWaitContext& context) const
{
	return !context.recording;
}

std::string RecordWait::describe(const DebugWaitContext& context) const
{
	return with_hint("wait record", context);
}

std::optional<std::string> RecordWait::failed(const DebugWaitContext& context) const
{
	// Armed, but its rebuild was dropped (or none was ever queued): only a
	// match started by hand from the menus would start it.
	if (context.record_armed && !context.build_pending)
	{
		return std::string("no match is queued to start the armed recording "
			"('record stop' disarms it)");
	}
	return std::nullopt;
}
#endif
