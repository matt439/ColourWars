#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_script_runner.h"
#include "game/debug/debug_log.h"

DebugStatus DebugScriptRunner::push(Frame frame, bool force)
{
	if (!force && !this->frames_.empty() && !this->executing_)
	{
		const std::string running = this->wait_ != nullptr ? this->wait_at_.where() :
			this->frames_.back().name;
		return DebugStatus::failure("exec: " + running + " is running ('exec.stop' first)");
	}
	// Files nest; the launch arguments under them do not count.
	const std::size_t files = static_cast<std::size_t>(std::count_if(this->frames_.begin(),
		this->frames_.end(), [](const Frame& open) { return !open.keep_going; }));
	if (files >= max_depth)
	{
		return DebugStatus::failure("exec: " + frame.name + " would nest deeper than " +
			std::to_string(max_depth) + " scripts");
	}
	for (const Frame& open : this->frames_)
	{
		if (open.name == frame.name)
		{
			return DebugStatus::failure("exec: " + frame.name + " is already running "
				"(a script cannot exec itself)");
		}
	}
	this->frames_.push_back(std::move(frame));
	return DebugStatus::success();
}

DebugStatus DebugScriptRunner::set_wait(std::unique_ptr<DebugWait> wait)
{
	if (!this->executing_)
	{
		return DebugStatus::failure("wait: only in a script (exec, autoexec.cfg or launch "
			"arguments)");
	}
	this->wait_ = std::move(wait);
	this->wait_at_ = this->current_;
	return DebugStatus::success();
}

DebugStatus DebugScriptRunner::tick(const Execute& execute, const Context& context,
	const Blocked& blocked, DebugLog& log)
{
	DebugStatus combined;
	for (int budget = 0; budget < max_statements_per_tick;)
	{
		if (this->wait_ != nullptr)
		{
			const DebugWaitContext now = context();
			if (const std::optional<std::string> failure = this->wait_->failed(now))
			{
				log.error("script " + this->wait_at_.where() + ": " + *failure + "; stopped");
				this->wait_.reset();
				combined.ok = false;
				this->unwind_after_error(log);
				continue;
			}
			if (!this->wait_->satisfied(now))
			{
				break;
			}
			this->wait_.reset();
		}
		if (this->frames_.empty())
		{
			break;
		}
		this->blocked_ = blocked && blocked();
		if (this->blocked_)
		{
			break;
		}
		while (!this->frames_.empty() &&
			this->frames_.back().next >= this->frames_.back().statements.size())
		{
			if (!this->frames_.back().keep_going)
			{
				log.info("script " + this->frames_.back().name + " done");
			}
			this->frames_.pop_back();
		}
		if (this->frames_.empty())
		{
			break;
		}

		Frame& frame = this->frames_.back();
		const DebugScriptStatement statement = frame.statements[frame.next++];
		const bool keep_going = frame.keep_going;
		log.write(DebugSeverity::echo, statement.line > 0 ?
			statement.where() + "] " + statement.text : statement.origin + ": " + statement.text);
		this->current_ = statement;
		this->executing_ = true;
		DebugStatus status;
		try
		{
			status = execute(statement);
		}
		catch (const std::exception& e)
		{
			status = DebugStatus::failure(std::string("exception: ") + e.what());
		}
		this->executing_ = false;
		++budget;
		combined.close_console = combined.close_console || status.close_console;
		if (!status.ok)
		{
			combined.ok = false;
			if (!keep_going && !this->frames_.empty())
			{
				log.error("script " + statement.where() + " stopped: " + status.message);
				this->wait_.reset();
				this->unwind_after_error(log);
			}
		}
	}
	return combined;
}

void DebugScriptRunner::unwind_after_error(DebugLog& log)
{
	// Back to the nearest frame that keeps going (the launch arguments), which
	// then runs its next statement; with none, the stack is empty.
	while (!this->frames_.empty() && !this->frames_.back().keep_going)
	{
		this->frames_.pop_back();
	}
	if (!this->frames_.empty())
	{
		log.info("script: continuing with the " + this->frames_.back().name + " statements");
	}
}

bool DebugScriptRunner::should_stop(const DebugWaitContext& context) const
{
	return this->wait_ != nullptr && this->wait_->should_stop(context);
}

void DebugScriptRunner::stop(const std::string& reason, DebugLog& log)
{
	if (this->frames_.empty())
	{
		return;
	}
	const DebugScriptStatement& at = this->wait_ != nullptr ? this->wait_at_ : this->current_;
	log.warning("script stopped at " + at.where() + " (" + reason + ")");
	this->frames_.clear();
	this->wait_.reset();
	this->blocked_ = false;
}

std::string DebugScriptRunner::waiting(const DebugWaitContext& context) const
{
	if (this->frames_.empty())
	{
		return {};
	}
	if (this->wait_ != nullptr)
	{
		return this->wait_at_.where() + " " + this->wait_->describe(context);
	}
	if (this->blocked_)
	{
		const std::string hint = debug_wait_hint(context);
		return this->current_.where() + " screenshot pending" + (hint.empty() ? "" : "; " + hint);
	}
	return {};
}

std::vector<std::string> DebugScriptRunner::stack_lines(const DebugWaitContext& context) const
{
	std::vector<std::string> lines;
	for (auto frame = this->frames_.rbegin(); frame != this->frames_.rend(); ++frame)
	{
		lines.push_back("  " + frame->name + " (" + std::to_string(frame->next) + " of " +
			std::to_string(frame->statements.size()) + " statements run)");
	}
	const std::string wait = this->waiting(context);
	if (!wait.empty())
	{
		lines.push_back("  " + wait);
	}
	return lines;
}
#endif
