#include "game/pch.h"
#include "game/input/rumble.h"
#include "game/config/content_value.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    bool same_motors(const RumbleMotors& left, const RumbleMotors& right) noexcept
    {
        return left.low_frequency == right.low_frequency &&
            left.high_frequency == right.high_frequency;
    }

    bool silent(const RumbleMotors& motors) noexcept
    {
        return motors.low_frequency == 0.0f && motors.high_frequency == 0.0f;
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
            throw std::invalid_argument("Rumble needs finite motor amplitudes in [0,1] and pulse durations in [0," +
                content_value::number_text(maximum_rumble_pulse_seconds) + "] seconds.");
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
        throw std::out_of_range("Rumble event is outside the " + std::to_string(rumble_event_count) +
            " supported cues.");
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
    const RumblePulse& pulse = this->profile_[index];
    this->slots_[slot].remaining[index] = static_cast<double>(pulse.duration_seconds);
    // An audible cue always renews the request, even at an unchanged output and
    // deadline: the engine's deadline runs on wall time and this policy's on
    // game time, so after a stall the pad may already have stopped.
    this->publish(slot, pulse.duration_seconds > 0.0f && !silent(pulse.motors));
}

void Rumble::update(float dt)
{
    if (!std::isfinite(dt) || dt < 0.0f)
    {
        throw std::invalid_argument("Rumble update needs a finite nonnegative duration.");
    }
    for (std::size_t slot = 0; slot < this->slots_.size(); ++slot)
    {
        Slot& target = this->slots_[slot];
        for (double& remaining : target.remaining)
        {
            remaining = std::max(0.0, remaining - static_cast<double>(dt));
        }
        // The same arithmetic as the pulse timers, so a request and the pulse
        // that set its seconds expire together and nothing is resent early.
        target.sent_seconds = std::max(0.0, target.sent_seconds - static_cast<double>(dt));
        this->publish(slot, false);
    }
}

void Rumble::clear_slot(std::size_t slot, bool force_output) noexcept
{
    Slot& target = this->slots_[slot];
    target.remaining.fill(0.0);
    const RumbleMotors zero;
    if (force_output || !same_motors(target.output, zero))
    {
        this->sink_.set_vibration(static_cast<int>(slot), zero, 0.0f);
    }
    target.output = zero;
    target.sent_seconds = 0.0;
}

void Rumble::publish(std::size_t slot, bool renew) noexcept
{
    Slot& target = this->slots_[slot];
    RumbleMotors output;
    // How long the output stays nonzero: the longest audible pulse left.
    double hold = 0.0;
    if (this->slot_permits(slot))
    {
        for (std::size_t event = 0; event < rumble_event_count; ++event)
        {
            const RumbleMotors& motors = this->profile_[event].motors;
            if (target.remaining[event] > 0.0)
            {
                output.low_frequency = std::max(output.low_frequency, motors.low_frequency);
                output.high_frequency = std::max(output.high_frequency, motors.high_frequency);
                if (!silent(motors)) { hold = std::max(hold, target.remaining[event]); }
            }
        }
    }
    if (silent(output)) { hold = 0.0; }
    if (!same_motors(target.output, output) || hold > target.sent_seconds || (renew && !silent(output)))
    {
        // Profiles bound hold by maximum_rumble_pulse_seconds; the floor keeps
        // a vanishing remainder a positive request rather than a refused one.
        const float seconds = silent(output) ? 0.0f :
            std::clamp(static_cast<float>(hold), (std::numeric_limits<float>::min)(), maximum_rumble_pulse_seconds);
        this->sink_.set_vibration(static_cast<int>(slot), output, seconds);
        target.output = output;
        target.sent_seconds = hold;
    }
}

void Rumble::stop_all() noexcept
{
    for (std::size_t slot = 0; slot < this->slots_.size(); ++slot)
    {
        this->clear_slot(slot, true);
    }
}
