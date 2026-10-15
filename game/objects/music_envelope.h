#pragma once

#include <algorithm>
#include <cmath>

// A client-side linear gain ramp. Retarget from the current value so an
// interrupted duck/restore stays continuous; repeated targets keep their clock.
class MusicEnvelope
{
public:
	explicit MusicEnvelope(float gain = 0.0f) : gain_(gain), start_(gain), target_(gain) {}
	void fade_to(float gain, float duration)
	{
		if (gain == this->target_) { return; }
		this->start_ = this->gain_;
		this->target_ = gain;
		this->duration_ = duration;
		this->elapsed_ = 0.0f;
	}
	void advance(float dt)
	{
		if (!(dt > 0.0f) || !std::isfinite(dt) || this->gain_ == this->target_) { return; }
		this->elapsed_ = std::min(this->elapsed_ + dt, this->duration_);
		const float ratio = this->elapsed_ / this->duration_;
		this->gain_ = ratio >= 1.0f ? this->target_ :
			this->start_ + (this->target_ - this->start_) * ratio;
	}
	float gain() const { return this->gain_; }
private:
	float gain_ = 0.0f;
	float start_ = 0.0f;
	float target_ = 0.0f;
	float duration_ = 1.0f;
	float elapsed_ = 0.0f;
};
