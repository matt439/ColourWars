#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

// The perf overlay's numbers: the last `capacity` rendered match frames, each
// with the interval since the previous one, the CPU time of the game's own
// Level::update calls in it and of its own match draw. Fed with explicit
// timestamps in milliseconds, so it can be tested without a clock. The frame
// interval includes intervening submit/Present wall time indirectly; separate
// Present durations and GPU/scan-out timings remain engine report 14.
// Debug-only bodies.
class DebugPerf
{
public:
	static constexpr std::size_t capacity = 240;
	static constexpr std::size_t max_capture_frames = 36000;

	struct Frame
	{
		double frame_ms = 0.0;
		double update_ms = 0.0;
		double draw_ms = 0.0;
		int updates = 0;
		int objects = 0;
		int projectiles = 0;
	};

	struct Series
	{
		double min = 0.0;
		double avg = 0.0;
		double max = 0.0;
	};

	// One Level::update took `ms`; it belongs to the next committed frame.
	void add_update(double ms);
	// The top of the match draw: the interval since the previous begin is
	// that frame's frame_ms.
	void begin_draw(double now_ms);
	// The current draw's interval, frozen at begin_draw for per-view readers.
	// Zero until two consecutive eligible draws, and outside an open draw.
	double current_frame_ms() const;
	// The end of the match draw: commits the frame. A draw with no previous
	// begin (the first, or the first after break_interval) has no interval:
	// it only starts the next one, and commits nothing.
	void end_draw(double now_ms);
	// Time not spent in match frames follows (a build, the menus, a page over
	// the match, a screenshot's redraw): the next draw starts a new interval
	// rather than timing the gap, and updates so far are forgotten.
	void break_interval();
	void set_counts(int objects, int projectiles);
	// A bounded chronological recording, independent of the overlay ring.
	// The first draw primes the interval. Export happens after recording ends.
	bool start_capture(std::size_t target);
	bool capture_active() const { return this->capture_active_; }
	bool capture_ready() const { return this->capture_target_ != 0 && !this->capture_active_; }
	bool capture_complete() const { return this->capture_ready() && this->capture_reason_ == "complete"; }
	const std::vector<Frame>& captured_frames() const { return this->capture_frames_; }
	std::size_t capture_target() const { return this->capture_target_; }
	const std::string& capture_reason() const { return this->capture_reason_; }
	void discard_capture();

	std::size_t size() const { return this->size_; }
	// 0 is the newest.
	const Frame& at(std::size_t age) const;
	Series frame() const;
	Series update() const;
	Series draw() const;
	int objects() const { return this->objects_; }
	int projectiles() const { return this->projectiles_; }

	// "frame 16.6 / 16.7 / 18.0 ms  update ..." and "objects 182  projectiles 12".
	std::vector<std::string> lines() const;
	void reset();

private:
	std::array<Frame, capacity> frames_{};
	std::size_t next_ = 0;
	std::size_t size_ = 0;
	double pending_update_ms_ = 0.0;
	int pending_updates_ = 0;
	double draw_begin_ = -1.0;
	double last_begin_ = -1.0;
	int objects_ = 0;
	int projectiles_ = 0;
	std::vector<Frame> capture_frames_;
	std::size_t capture_target_ = 0;
	bool capture_active_ = false;
	std::string capture_reason_;

	template <typename Field>
	Series series(Field field) const;
};

// QueryPerformanceCounter in milliseconds.
double perf_now_ms();
