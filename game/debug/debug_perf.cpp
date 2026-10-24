#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "game/debug/debug_perf.h"
#include <cstdio>
#include <cmath>
#include <limits>

namespace
{
	bool valid_phases(const labrador::FrameTimings& snapshot)
	{
		return snapshot.begin_frame_time.count() >= 0 && snapshot.submit_time.count() >= 0 &&
			snapshot.end_frame_time.count() >= 0;
	}
}

void DebugPerf::add_update(double ms)
{
	if (ms < 0.0 || !std::isfinite(ms)) { return; }
	this->pending_update_ms_ += ms;
	++this->pending_updates_;
}

void DebugPerf::begin_draw(double now_ms)
{
	// Clock-only seam for synthetic fixtures, not a paired runtime frame.
	if (this->has_engine_frame_) { this->break_interval(); }
	this->drawing_application_frame_ = 0;
	this->begin_draw_clock(now_ms);
}

void DebugPerf::begin_draw(double now_ms, const labrador::FrameTimings& snapshot)
{
	if (!valid_phases(snapshot) || snapshot.frame == (std::numeric_limits<std::uint64_t>::max)() ||
		(this->has_engine_frame_ && snapshot.frame != this->last_engine_frame_ + 1))
	{
		this->break_interval();
		return;
	}
	this->begin_draw_clock(now_ms);
	if (this->draw_begin_ < 0.0) { return; }
	this->observe_frame_timings(snapshot);
	this->drawing_application_frame_ = snapshot.frame + 1;
	this->last_engine_frame_ = snapshot.frame;
	this->has_engine_frame_ = true;
}

void DebugPerf::observe_frame_timings(const labrador::FrameTimings& snapshot)
{
	if (this->pending_application_frame_ == 0) { return; }
	// Catch-up updates can see the still-in-flight frame's predecessor.
	if (snapshot.frame == this->pending_application_frame_ - 1) { return; }
	if (snapshot.frame != this->pending_application_frame_ || !valid_phases(snapshot))
	{
		this->break_interval();
		return;
	}
	Frame& frame = this->frames_[(this->next_ + capacity - 1) % capacity];
	frame.application_frame = snapshot.frame;
	frame.begin_frame_ms = std::chrono::duration<double, std::milli>(snapshot.begin_frame_time).count();
	frame.submit_ms = std::chrono::duration<double, std::milli>(snapshot.submit_time).count();
	frame.end_frame_ms = std::chrono::duration<double, std::milli>(snapshot.end_frame_time).count();
	if (this->pending_capture_frame_)
	{
		this->capture_frames_.back() = frame;
		if (this->capture_frames_.size() == this->capture_target_)
		{
			this->capture_active_ = false;
			this->capture_reason_ = "complete";
		}
	}
	this->pending_application_frame_ = 0;
	this->pending_capture_frame_ = false;
}

void DebugPerf::begin_draw_clock(double now_ms)
{
	if (!std::isfinite(now_ms) || now_ms < 0.0 ||
		(this->last_begin_ >= 0.0 && now_ms <= this->last_begin_))
	{
		this->break_interval();
		return;
	}
	this->draw_begin_ = now_ms;
}

double DebugPerf::current_frame_ms() const
{
	return this->last_begin_ >= 0.0 && this->draw_begin_ > this->last_begin_ ?
		this->draw_begin_ - this->last_begin_ : 0.0;
}

void DebugPerf::end_draw(double now_ms)
{
	if (this->draw_begin_ < 0.0)
	{
		return;
	}
	if (!std::isfinite(now_ms) || now_ms < this->draw_begin_)
	{
		this->break_interval();
		return;
	}
	if (this->last_begin_ < 0.0)
	{
		// No interval to measure: this draw only starts the next one.
		this->last_begin_ = this->draw_begin_;
		this->draw_begin_ = -1.0;
		this->drawing_application_frame_ = 0;
		this->pending_update_ms_ = 0.0;
		this->pending_updates_ = 0;
		return;
	}
	Frame frame;
	const std::uint64_t application_frame = this->drawing_application_frame_;
	frame.frame_ms = this->draw_begin_ - this->last_begin_;
	frame.draw_ms = now_ms - this->draw_begin_;
	frame.update_ms = this->pending_update_ms_;
	frame.updates = this->pending_updates_;
	frame.objects = this->objects_;
	frame.projectiles = this->projectiles_;
	this->last_begin_ = this->draw_begin_;
	this->draw_begin_ = -1.0;
	this->drawing_application_frame_ = 0;
	this->pending_update_ms_ = 0.0;
	this->pending_updates_ = 0;

	this->frames_[this->next_] = frame;
	this->next_ = (this->next_ + 1) % capacity;
	this->size_ = std::min(this->size_ + 1, capacity);
	this->pending_application_frame_ = application_frame;
	this->pending_capture_frame_ = false;
	if (this->capture_active_ && this->capture_frames_.size() < this->capture_target_)
	{
		this->capture_frames_.push_back(frame);
		this->pending_capture_frame_ = application_frame != 0;
		if (this->capture_frames_.size() == this->capture_target_ && application_frame == 0)
		{
			this->capture_active_ = false;
			this->capture_reason_ = "complete";
		}
	}
}

void DebugPerf::break_interval(const labrador::FrameTimings& snapshot)
{
	this->observe_frame_timings(snapshot);
	this->break_interval();
}

void DebugPerf::break_interval()
{
	if (this->capture_active_)
	{
		this->capture_active_ = false;
		this->capture_reason_ = "interrupted";
	}
	this->last_begin_ = -1.0;
	this->draw_begin_ = -1.0;
	this->drawing_application_frame_ = 0;
	this->pending_application_frame_ = 0;
	this->pending_capture_frame_ = false;
	this->last_engine_frame_ = 0;
	this->has_engine_frame_ = false;
	this->pending_update_ms_ = 0.0;
	this->pending_updates_ = 0;
}

bool DebugPerf::start_capture(std::size_t target)
{
	if (target == 0 || target > max_capture_frames || this->capture_target_ != 0) { return false; }
	this->break_interval();
	this->capture_frames_.clear();
	this->capture_frames_.reserve(target);
	this->capture_target_ = target;
	this->capture_active_ = true;
	this->capture_reason_.clear();
	return true;
}

void DebugPerf::discard_capture()
{
	this->pending_capture_frame_ = false;
	this->capture_frames_.clear();
	this->capture_target_ = 0;
	this->capture_active_ = false;
	this->capture_reason_.clear();
}

void DebugPerf::set_counts(int objects, int projectiles)
{
	this->objects_ = objects;
	this->projectiles_ = projectiles;
}

const DebugPerf::Frame& DebugPerf::at(std::size_t age) const
{
	return this->frames_[(this->next_ + capacity - 1 - age % capacity) % capacity];
}

template <typename Field>
DebugPerf::Series DebugPerf::series(Field field, bool engine_only) const
{
	Series result;
	if (this->size_ == 0)
	{
		return result;
	}
	double total = 0.0;
	std::size_t count = 0;
	for (std::size_t age = 0; age < this->size_; ++age)
	{
		if (engine_only && this->at(age).application_frame == 0) { continue; }
		const double value = field(this->at(age));
		result.min = count == 0 ? value : std::min(result.min, value);
		result.max = count == 0 ? value : std::max(result.max, value);
		total += value;
		++count;
	}
	result.avg = count == 0 ? 0.0 : total / static_cast<double>(count);
	return result;
}

DebugPerf::Series DebugPerf::frame() const
{
	return this->series([](const Frame& frame) { return frame.frame_ms; });
}

DebugPerf::Series DebugPerf::update() const
{
	return this->series([](const Frame& frame) { return frame.update_ms; });
}

DebugPerf::Series DebugPerf::draw() const
{
	return this->series([](const Frame& frame) { return frame.draw_ms; });
}

DebugPerf::Series DebugPerf::begin_frame() const
{
	return this->series([](const Frame& frame) { return frame.begin_frame_ms; }, true);
}

DebugPerf::Series DebugPerf::submit() const
{
	return this->series([](const Frame& frame) { return frame.submit_ms; }, true);
}

DebugPerf::Series DebugPerf::end_frame() const
{
	return this->series([](const Frame& frame) { return frame.end_frame_ms; }, true);
}

std::size_t DebugPerf::engine_samples() const
{
	std::size_t count = 0;
	for (std::size_t age = 0; age < this->size_; ++age)
	{
		if (this->at(age).application_frame != 0) { ++count; }
	}
	return count;
}

std::vector<std::string> DebugPerf::lines() const
{
	const auto format = [](const char* name, const Series& series)
		{
			char buffer[96] = {};
			std::snprintf(buffer, sizeof(buffer), "%-6s %6.2f %6.2f %6.2f", name,
				series.min, series.avg, series.max);
			return std::string(buffer);
		};
	double updates = 0.0;
	for (std::size_t age = 0; age < this->size_; ++age)
	{
		updates += this->at(age).updates;
	}
	char per_frame[48] = {};
	std::snprintf(per_frame, sizeof(per_frame), "  x%.2f a frame", this->size_ == 0 ? 0.0 :
		updates / static_cast<double>(this->size_));
	const std::size_t completed = this->engine_samples();
	const auto phase = [&](const char* name, const Series& values)
	{
		return completed == 0 ? std::string(name) + "  n/a" : format(name, values);
	};
	return {
		"CPU wall ms min    avg    max  (" + std::to_string(this->size_) + " frames)",
		format("frame", this->frame()),
		format("update", this->update()) + per_frame,
		format("draw", this->draw()),
		phase("begin", this->begin_frame()) + " (" + std::to_string(completed) + " engine frames)",
		phase("submit", this->submit()),
		phase("end", this->end_frame()),
		"objects " + std::to_string(this->objects_) + "  projectiles " +
			std::to_string(this->projectiles_),
	};
}

void DebugPerf::reset()
{
	*this = DebugPerf();
}

double perf_now_ms()
{
	static const double frequency = []
		{
			LARGE_INTEGER value{};
			QueryPerformanceFrequency(&value);
			return static_cast<double>(value.QuadPart);
		}();
	LARGE_INTEGER now{};
	QueryPerformanceCounter(&now);
	return static_cast<double>(now.QuadPart) * 1000.0 / frequency;
}
#endif
