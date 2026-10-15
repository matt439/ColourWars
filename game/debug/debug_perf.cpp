#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_perf.h"
#include <cstdio>
#include <cmath>

void DebugPerf::add_update(double ms)
{
	if (ms < 0.0 || !std::isfinite(ms)) { return; }
	this->pending_update_ms_ += ms;
	++this->pending_updates_;
}

void DebugPerf::begin_draw(double now_ms)
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
		this->pending_update_ms_ = 0.0;
		this->pending_updates_ = 0;
		return;
	}
	Frame frame;
	frame.frame_ms = this->draw_begin_ - this->last_begin_;
	frame.draw_ms = now_ms - this->draw_begin_;
	frame.update_ms = this->pending_update_ms_;
	frame.updates = this->pending_updates_;
	frame.objects = this->objects_;
	frame.projectiles = this->projectiles_;
	this->last_begin_ = this->draw_begin_;
	this->draw_begin_ = -1.0;
	this->pending_update_ms_ = 0.0;
	this->pending_updates_ = 0;

	this->frames_[this->next_] = frame;
	this->next_ = (this->next_ + 1) % capacity;
	this->size_ = std::min(this->size_ + 1, capacity);
	if (this->capture_active_)
	{
		this->capture_frames_.push_back(frame);
		if (this->capture_frames_.size() == this->capture_target_)
		{
			this->capture_active_ = false;
			this->capture_reason_ = "complete";
		}
	}
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
DebugPerf::Series DebugPerf::series(Field field) const
{
	Series result;
	if (this->size_ == 0)
	{
		return result;
	}
	result.min = field(this->at(0));
	result.max = result.min;
	double total = 0.0;
	for (std::size_t age = 0; age < this->size_; ++age)
	{
		const double value = field(this->at(age));
		result.min = std::min(result.min, value);
		result.max = std::max(result.max, value);
		total += value;
	}
	result.avg = total / static_cast<double>(this->size_);
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
	return {
		"CPU ms    min    avg    max  (" + std::to_string(this->size_) + " frames)",
		format("frame", this->frame()),
		format("update", this->update()) + per_frame,
		format("draw", this->draw()),
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
