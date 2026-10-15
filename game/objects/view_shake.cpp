#include "game/pch.h"
#include "game/objects/view_shake.h"

#include <numbers>

using namespace mattmath;

ViewShake::ViewShake(const ViewShakePresentation* presentation) :
    presentation_(presentation) {}

void ViewShake::update(float dt, bool enabled)
{
    if (dt <= 0.0f || !std::isfinite(dt)) { return; }
    if (!enabled) { this->clear(); return; }
    this->remaining_ = std::max(0.0f, this->remaining_ - dt);
    this->hurt_delay_ = std::max(0.0f, this->hurt_delay_ - dt);
    if (this->remaining_ == 0.0f) { this->amplitude_ = 0.0f; }
}

void ViewShake::impulse(float amplitude)
{
    // Replace a weaker active accent, never stack unbounded impulses. A lesser
    // event cannot extend a stronger one. Storage stays constant under 4P fire.
    if (this->presentation_ == nullptr || amplitude <= 0.0f ||
        (this->remaining_ > 0.0f && amplitude < this->amplitude_)) { return; }
    this->amplitude_ = amplitude;
    // Live tuning applies to the next impulse. An in-flight accent keeps its
    // envelope so reducing the duration cannot amplify the current offset.
    this->duration_ = this->presentation_->duration;
    this->cycles_ = this->presentation_->cycles;
    this->remaining_ = this->duration_;
}

void ViewShake::hurt()
{
    if (this->presentation_ == nullptr || this->hurt_delay_ > 0.0f) { return; }
    this->impulse(this->presentation_->hurt_amplitude);
    this->hurt_delay_ = this->presentation_->repeat_delay;
}

void ViewShake::eliminate()
{
    if (this->presentation_ != nullptr)
    {
        this->impulse(this->presentation_->elimination_amplitude);
    }
}

void ViewShake::land(float speed)
{
    if (this->presentation_ == nullptr || !std::isfinite(speed) ||
        speed <= this->presentation_->landing_min_speed) { return; }
    const float strength = std::clamp((speed - this->presentation_->landing_min_speed) /
        (this->presentation_->landing_full_speed - this->presentation_->landing_min_speed),
        0.0f, 1.0f);
    this->impulse(this->presentation_->landing_amplitude * strength);
}

void ViewShake::clear()
{
    this->remaining_ = 0.0f;
    this->amplitude_ = 0.0f;
    this->hurt_delay_ = 0.0f;
}

Vector2F ViewShake::offset() const
{
    if (this->presentation_ == nullptr || this->remaining_ <= 0.0f) { return Vector2F::ZERO; }
    const float remaining = this->remaining_ / this->duration_;
    const float phase = (1.0f - remaining) * this->cycles_ *
        2.0f * std::numbers::pi_v<float>;
    const float strength = this->amplitude_ * remaining * remaining;
    return Vector2F(std::cos(phase), std::sin(phase) * 0.5f) * strength;
}

float ViewShake::remaining() const { return this->remaining_; }
