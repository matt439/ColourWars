#pragma once

#include <optional>
#include <string>

// What a script's wait can see, built fresh by the director whenever a wait
// is checked (the statement before it may just have changed everything).
struct DebugWaitContext
{
	// Level updates since the director was made; never reset.
	long long updates = 0;
	// Hook frames on which a script could run (the top of a GameLevel or
	// MenuPage update, not under the debug menu or a modal console): what
	// `wait frames` counts, in the menus too.
	long long frames = 0;
	bool in_match = false;
	// The attached match's round is over: its updates have stopped for good
	// and its results (or end) page is up or about to be.
	bool round_over = false;
	// Attached, nothing queued to rebuild it, and its countdown is over.
	bool match_ready = false;
	// A match is coming: a queued stage or rebuild, or one being started.
	bool build_pending = false;
	// A replay is armed, or playing and not finished.
	bool replay_active = false;
	// A recording is armed or running.
	bool recording = false;
	// A recording is armed and has not started (it starts with the next build).
	bool record_armed = false;
	bool paused = false;
	bool window_active = true;
};

// What a script waits for. The runner checks it between updates; the
// director asks should_stop after every Level update, so a fast-forwarded
// step never runs past the frame a script asked for.
class DebugWait
{
public:
	virtual ~DebugWait() = default;

	virtual bool satisfied(const DebugWaitContext& context) const = 0;
	// "wait 30 updates (12 left)", for the status strip and `status`.
	virtual std::string describe(const DebugWaitContext& context) const = 0;
	// A reason when the wait can never be satisfied: the script stops.
	virtual std::optional<std::string> failed(const DebugWaitContext& context) const;
	// Stop the debug clock's remaining runs of this step here.
	virtual bool should_stop(const DebugWaitContext& context) const;

protected:
	DebugWait() = default;
	DebugWait(const DebugWait&) = default;
	DebugWait& operator=(const DebugWait&) = default;
};

// `wait <n>` and `wait <x>s`: Level updates, so it follows timescale, pause
// and step exactly.
class UpdatesWait final : public DebugWait
{
public:
	UpdatesWait(long long target, long long count) : target_(target), count_(count) {}

	bool satisfied(const DebugWaitContext& context) const override;
	std::string describe(const DebugWaitContext& context) const override;
	std::optional<std::string> failed(const DebugWaitContext& context) const override;
	bool should_stop(const DebugWaitContext& context) const override;

private:
	long long target_ = 0;
	long long count_ = 0;
};

// `wait frames <n>`: hook frames, the only wait that counts in the menus.
class FramesWait final : public DebugWait
{
public:
	FramesWait(long long target, long long count) : target_(target), count_(count) {}

	bool satisfied(const DebugWaitContext& context) const override;
	std::string describe(const DebugWaitContext& context) const override;

private:
	long long target_ = 0;
	long long count_ = 0;
};

// `wait match`: the queued match is built and its countdown is over.
class MatchWait final : public DebugWait
{
public:
	bool satisfied(const DebugWaitContext& context) const override;
	std::string describe(const DebugWaitContext& context) const override;
	std::optional<std::string> failed(const DebugWaitContext& context) const override;
	bool should_stop(const DebugWaitContext& context) const override;
};

// `wait replay`: the running or armed replay has finished (and holds its
// last frame, paused).
class ReplayWait final : public DebugWait
{
public:
	bool satisfied(const DebugWaitContext& context) const override;
	std::string describe(const DebugWaitContext& context) const override;
};

// `wait record`: the running or armed recording has been saved.
class RecordWait final : public DebugWait
{
public:
	bool satisfied(const DebugWaitContext& context) const override;
	std::string describe(const DebugWaitContext& context) const override;
	std::optional<std::string> failed(const DebugWaitContext& context) const override;
};

// Why nothing is moving, for a wait's description: "the clock is paused",
// "the window is inactive", or empty.
std::string debug_wait_hint(const DebugWaitContext& context);
