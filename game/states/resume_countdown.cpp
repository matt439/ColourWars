#include "game/pch.h"
#include "game/states/resume_countdown.h"

void ResumeCountdown::begin(float duration)
{
	this->cancel();
	if (!std::isfinite(duration) || duration <= 0.0f) { return; }
	this->remaining_ = duration;
	this->held_.fill(HeldActions{ true, true, true, true });
}

void ResumeCountdown::cancel()
{
	this->remaining_ = 0.0;
	this->held_ = {};
}

bool ResumeCountdown::active() const { return this->remaining_ > 0.0; }

float ResumeCountdown::remaining() const
{
	return static_cast<float>(this->remaining_);
}

bool ResumeCountdown::advance(float dt)
{
	if (!this->active() || !std::isfinite(dt) || dt <= 0.0f) { return false; }
	this->remaining_ = std::max(0.0, this->remaining_ - static_cast<double>(dt));
	return !this->active();
}

void ResumeCountdown::filter_inputs(std::vector<SlotInput>& inputs)
{
	const bool counting = this->active();
	const size_t slots = std::min(inputs.size(), this->held_.size());
	for (size_t slot = 0; slot < slots; ++slot)
	{
		SlotInput& input = inputs[slot];
		HeldActions& held = this->held_[slot];
		const bool jump = input.jump_held || input.jump_pressed;
		const bool fire = input.primary_shoot;
		const bool drop = input.drop_through;
		if (counting)
		{
			// Retain raw final-sample state so a held action cannot become a
			// surprise action when the owner allows its next Level update.
			held.jump = jump;
			held.fire = fire;
			held.drop = drop;
		}
		else
		{
			if (!jump) { held.jump = false; }
			if (!fire) { held.fire = false; }
			if (!drop) { held.drop = false; }
		}
		if (counting || held.jump) { input.jump_pressed = false; }
		// Keep jump_held: an airborne held jump resumes its existing arc.
		// Level::suspend has already cleared buffers and stale press arming.
		if (counting || held.fire) { input.primary_shoot = false; }
		if (counting || held.drop) { input.drop_through = false; }

		if (held.pause && input.toggle_pause_menu)
		{
			input.toggle_pause_menu = false;
			held.pause = false;
		}
		else if (!input.pause_held)
		{
			// A neutral sample rearms pause even while the count is running.
			held.pause = false;
		}
	}
}
