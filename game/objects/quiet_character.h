#pragma once

#include "game/presentation.h"
#include "engine/math/scalar.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

// One cosmetic clock per roster member, advanced after resolved contacts.
// It never consumes a gameplay random draw or changes movement/weapon state.
class QuietCharacter final
{
public:
    struct State
    {
        float breath = 0.0f;
        float lean = 0.0f;
        bool blinking = false;
        bool operator==(const State&) const = default;
    };

    QuietCharacter(const FigurePresentation* figure, int identity) :
        figure_(figure), random_(0x9e3779b9u ^ (static_cast<uint32_t>(identity) + 1u) * 0x85ebca6bu)
    {
        phase_ = this->next_fraction();
        shift_phase_ = this->next_fraction();
        if (figure_ != nullptr) { this->schedule_blink(); }
    }

    void update(bool alive, bool supported, bool resting, bool acting,
        float previous_velocity, float velocity, float dt)
    {
        if (!(dt > 0.0f) || !std::isfinite(dt)) { return; }
        if (figure_ == nullptr || !figure_->quiet.enabled || !alive)
        {
            this->reset();
            return;
        }
        const auto& look = figure_->quiet;
        phase_ = std::fmod(phase_ + std::fmod(dt, look.breath_period) / look.breath_period, 1.0f);
        shift_phase_ = std::fmod(shift_phase_ + std::fmod(dt, look.shift_period) / look.shift_period, 1.0f);
        const bool at_rest = supported && resting && !acting;
        rest_age_ = at_rest ? std::min(look.settle_time, rest_age_ + dt) : 0.0f;
        const bool settled = at_rest && rest_age_ >= look.settle_time;
        const float blend = std::min(1.0f, dt / look.lean_settle_time);
        const float acceleration = (velocity - previous_velocity) / dt;
        float target = supported && !acting ? std::clamp(acceleration /
            look.lean_acceleration, -1.0f, 1.0f) * look.lean_distance : 0.0f;
        if (settled)
        {
            target += std::sin(shift_phase_ * 2.0f * mattmath::PI) * look.shift_distance;
            state_.breath = (1.0f - std::cos(phase_ * 2.0f * mattmath::PI)) *
                0.5f * look.breath_height;
            blink_wait_ -= dt;
            if (blink_remaining_ > 0.0f)
            {
                blink_remaining_ = std::max(0.0f, blink_remaining_ - dt);
            }
            else if (blink_wait_ <= 0.0f)
            {
                blink_remaining_ = look.blink_duration;
                this->schedule_blink();
            }
        }
        else
        {
            state_.breath = 0.0f;
            blink_remaining_ = 0.0f;
        }
        state_.lean += (target - state_.lean) * blend;
        if (!supported || acting) { state_.lean = 0.0f; }
        state_.blinking = settled && blink_remaining_ > 0.0f;
    }

    void reset()
    {
        state_ = {};
        rest_age_ = 0.0f;
        blink_remaining_ = 0.0f;
    }

    const State& state() const { return state_; }

    // Breathing compresses from the feet up. A small asymmetric width change
    // suggests weight shifting without rotation lifting a foot off its baseline.
    mattmath::RectangleF rectangle(const mattmath::RectangleF& body) const
    {
        if (figure_ == nullptr || !figure_->quiet.enabled) { return body; }
        const float compression = state_.breath * body.height / figure_->canvas_size.y;
        const float lean = state_.lean * body.width / figure_->canvas_size.x;
        return { body.x + (std::abs(lean) + lean) / 2.0f, body.y + compression,
            body.width - std::abs(lean), body.height - compression };
    }

private:
    const FigurePresentation* figure_ = nullptr;
    State state_;
    uint32_t random_;
    float phase_ = 0.0f;
    float shift_phase_ = 0.0f;
    float rest_age_ = 0.0f;
    float blink_wait_ = 0.0f;
    float blink_remaining_ = 0.0f;

    float next_fraction()
    {
        random_ = random_ * 1664525u + 1013904223u;
        return static_cast<float>(random_ >> 8) / 16777216.0f;
    }

    void schedule_blink()
    {
        const auto& look = figure_->quiet;
        blink_wait_ = look.blink_min_interval + this->next_fraction() *
            (look.blink_max_interval - look.blink_min_interval);
    }
};
