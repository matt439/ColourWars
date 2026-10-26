#include "game/pch.h"
#include "game/rumble.h"
#include <cmath>
#include <stdexcept>

namespace
{
    bool same_motors(const RumbleMotors& left, const RumbleMotors& right) noexcept
    {
        return left.low_frequency == right.low_frequency &&
            left.high_frequency == right.high_frequency;
    }
}

void Rumble::validate_profile(const RumbleProfile& profile)
{
    for (const RumblePulse& pulse : profile)
    {
        if (!std::isfinite(pulse.motors.low_frequency) ||
            !std::isfinite(pulse.motors.high_frequency) ||
            pulse.motors.low_frequency < 0.0f || pulse.motors.low_frequency > 1.0f ||
            pulse.motors.high_frequency < 0.0f || pulse.motors.high_frequency > 1.0f ||
            !std::isfinite(pulse.duration_seconds) || pulse.duration_seconds < 0.0f ||
            pulse.duration_seconds > maximum_rumble_pulse_seconds)
        {
            throw std::invalid_argument("Rumble needs finite motor amplitudes in [0,1] and pulse durations in [0,1] seconds.");
        }
    }
}

void Rumble::validate_slot(int input_slot)
{
    if (input_slot < 0 || input_slot >= input_slots::count)
    {
        throw std::out_of_range("Rumble input slot is outside the physical pads and keyboard seat.");
    }
}

std::size_t Rumble::event_index(RumbleEvent event)
{
    const std::size_t index = static_cast<std::size_t>(event);
    if (index >= rumble_event_count)
    {
        throw std::out_of_range("Rumble event is outside the four supported cues.");
    }
    return index;
}

Rumble::Rumble(RumbleSink& sink, const RumbleProfile& profile) :
    sink_(sink), profile_(profile)
{
    validate_profile(profile);
}

Rumble::~Rumble() noexcept
{
    this->stop_all();
}

bool Rumble::session_permits() const noexcept
{
    return this->session_.match_active && this->session_.focused && !this->session_.paused;
}

bool Rumble::slot_permits(std::size_t slot) const noexcept
{
    const RumbleSlotState& state = this->slots_[slot].state;
    return this->session_permits() && state.connected && state.enabled && !state.bot && !state.replay;
}

void Rumble::set_session(const RumbleSessionState& state) noexcept
{
    const bool was_permitted = this->session_permits();
    this->session_ = state;
    if (was_permitted && !this->session_permits()) { this->stop_all(); }
}

void Rumble::set_slot(int input_slot, const RumbleSlotState& state)
{
    validate_slot(input_slot);
    if (!input_slots::is_pad(input_slot)) { return; }
    const std::size_t slot = static_cast<std::size_t>(input_slot);
    const bool was_permitted = this->slot_permits(slot);
    this->slots_[slot].state = state;
    if (!this->slot_permits(slot)) { this->clear_slot(slot, was_permitted); }
}

void Rumble::trigger(int input_slot, RumbleEvent event)
{
    validate_slot(input_slot);
    const std::size_t index = event_index(event);
    if (!input_slots::is_pad(input_slot)) { return; }
    const std::size_t slot = static_cast<std::size_t>(input_slot);
    if (!this->slot_permits(slot)) { return; }
    this->slots_[slot].remaining[index] = static_cast<double>(this->profile_[index].duration_seconds);
    this->publish(slot);
}

void Rumble::update(float dt)
{
    if (!std::isfinite(dt) || dt < 0.0f)
    {
        throw std::invalid_argument("Rumble update needs a finite nonnegative duration.");
    }
    for (std::size_t slot = 0; slot < this->slots_.size(); ++slot)
    {
        for (double& remaining : this->slots_[slot].remaining)
        {
            remaining = std::max(0.0, remaining - static_cast<double>(dt));
        }
        this->publish(slot);
    }
}

void Rumble::clear_slot(std::size_t slot, bool force_output) noexcept
{
    Slot& target = this->slots_[slot];
    target.remaining.fill(0.0);
    const RumbleMotors zero;
    if (force_output || !same_motors(target.output, zero))
    {
        this->sink_.set_vibration(static_cast<int>(slot), zero);
    }
    target.output = zero;
}

void Rumble::publish(std::size_t slot) noexcept
{
    Slot& target = this->slots_[slot];
    RumbleMotors output;
    if (this->slot_permits(slot))
    {
        for (std::size_t event = 0; event < rumble_event_count; ++event)
        {
            if (target.remaining[event] > 0.0)
            {
                output.low_frequency = std::max(output.low_frequency, this->profile_[event].motors.low_frequency);
                output.high_frequency = std::max(output.high_frequency, this->profile_[event].motors.high_frequency);
            }
        }
    }
    if (!same_motors(target.output, output))
    {
        this->sink_.set_vibration(static_cast<int>(slot), output);
        target.output = output;
    }
}

void Rumble::stop_all() noexcept
{
    for (std::size_t slot = 0; slot < this->slots_.size(); ++slot)
    {
        this->clear_slot(slot, true);
    }
}
