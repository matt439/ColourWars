#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_clock.h"

void DebugClock::set_timescale(float scale)
{
	this->timescale_ = std::clamp(scale, min_timescale, max_timescale);
}

float DebugClock::timescale() const
{
	return this->timescale_;
}

void DebugClock::set_paused(bool paused)
{
	this->paused_ = paused;
	if (!paused)
	{
		this->pending_steps_ = 0;
	}
}

bool DebugClock::paused() const
{
	return this->paused_;
}

void DebugClock::request_steps(int count)
{
	this->paused_ = true;
	this->pending_steps_ = std::clamp(this->pending_steps_ + count, 0, max_steps);
}

int DebugClock::pending_steps() const
{
	return this->pending_steps_;
}

int DebugClock::updates_this_step()
{
	if (this->paused_)
	{
		if (this->pending_steps_ > 0)
		{
			--this->pending_steps_;
			return 1;
		}
		return 0;
	}

	this->accumulator_ += this->timescale_;
	const int runs = static_cast<int>(std::floor(this->accumulator_ + 1.0e-4f));
	this->accumulator_ = std::max(0.0f, this->accumulator_ - static_cast<float>(runs));
	return std::min(runs, max_runs);
}

std::vector<SlotInput> DebugClock::latch(
	const std::vector<SlotInput>& inputs, int runs)
{
	if (this->held_edges_.size() != inputs.size())
	{
		this->held_edges_.assign(inputs.size(), SlotInput());
	}

	if (runs <= 0)
	{
		for (std::size_t i = 0; i < inputs.size(); ++i)
		{
			this->held_edges_[i].jump_pressed |= inputs[i].jump_pressed;
			this->held_edges_[i].toggle_debug |= inputs[i].toggle_debug;
			this->held_edges_[i].skip_intro |= inputs[i].skip_intro;
		}
		return inputs;
	}

	std::vector<SlotInput> result = inputs;
	for (std::size_t i = 0; i < result.size(); ++i)
	{
		result[i].jump_pressed |= this->held_edges_[i].jump_pressed;
		result[i].toggle_debug |= this->held_edges_[i].toggle_debug;
		result[i].skip_intro |= this->held_edges_[i].skip_intro;
		this->held_edges_[i] = SlotInput();
	}
	return result;
}

std::vector<SlotInput> DebugClock::without_edges(std::vector<SlotInput> inputs)
{
	for (SlotInput& input : inputs)
	{
		input.jump_pressed = false;
		input.toggle_debug = false;
		input.skip_intro = false;
	}
	return inputs;
}

void DebugClock::reset_accumulator()
{
	this->accumulator_ = 0.0f;
	this->held_edges_.clear();
}

void DebugClock::discard_input_edges()
{
	this->held_edges_.clear();
}

bool DebugClock::is_default() const
{
	return !this->paused_ && this->timescale_ == 1.0f;
}
#endif
