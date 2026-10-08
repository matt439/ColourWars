#pragma once

#include "game/debug/debug_entry.h"
#include "game/debug/debug_script.h"
#include "game/debug/debug_wait.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class DebugLog;

// Runs scripts: a stack of frames (launch arguments, autoexec.cfg, each
// `exec`), one statement at a time, stopping at a wait or a barrier. The
// director ticks it from its two hooks only, so a script always runs between
// updates, never inside one. Debug-only.
class DebugScriptRunner
{
public:
	static constexpr std::size_t max_depth = 8;
	static constexpr int max_statements_per_tick = 1000;

	using Execute = std::function<DebugStatus(const DebugScriptStatement&)>;
	using Context = std::function<DebugWaitContext()>;
	// True while the next statement must not run yet (a screenshot is still
	// pending). It may first try to clear itself (take the shot).
	using Blocked = std::function<bool()>;

	struct Frame
	{
		// "name.cfg", or "launch".
		std::string name;
		std::vector<DebugScriptStatement> statements;
		std::size_t next = 0;
		// Launch arguments: an error is logged and the next statement runs,
		// as it always has. A file stops at its first error.
		bool keep_going = false;
	};

	bool running() const { return !this->frames_.empty(); }
	// Inside tick(), running a statement: `wait` and a nested `exec` are legal.
	bool executing() const { return this->executing_; }
	std::size_t depth() const { return this->frames_.size(); }

	// Pushes a frame: nested from inside a running statement, or onto an
	// empty stack, or (`force`, for launch and autoexec) always. Refused with
	// a reason when another script is running, at max_depth, or when `name`
	// is already on the stack.
	DebugStatus push(Frame frame, bool force = false);

	// Called by `wait` while executing; the runner yields after the statement.
	DebugStatus set_wait(std::unique_ptr<DebugWait> wait);

	// Runs statements until a wait, a barrier, the budget or the end.
	// Returns the statuses OR-ed (ok only when every statement succeeded).
	DebugStatus tick(const Execute& execute, const Context& context,
		const Blocked& blocked, DebugLog& log);

	// After every Level update: stop the step's remaining runs here.
	bool should_stop(const DebugWaitContext& context) const;

	// Empties the stack, logging where it stopped and why.
	void stop(const std::string& reason, DebugLog& log);

	// "review_feedback_ab.cfg:14 wait 30 updates (12 left)" while a wait or a
	// barrier holds a script; empty otherwise.
	std::string waiting(const DebugWaitContext& context) const;

	// One line per frame, innermost first, for `status`.
	std::vector<std::string> stack_lines(const DebugWaitContext& context) const;

private:
	std::vector<Frame> frames_;
	std::unique_ptr<DebugWait> wait_;
	DebugScriptStatement wait_at_;
	// The statement running now, or the last one run (the barrier's owner).
	DebugScriptStatement current_;
	bool executing_ = false;
	bool blocked_ = false;

	void unwind_after_error(DebugLog& log);
};
