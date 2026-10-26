#pragma once

#include "game/rumble_sink.h"
#include <array>
#include <cmath>

// Every painter probe links this factory instead of rumble_xinput.cpp. Most
// modes discard intent; the rumble leaf attaches a bounded recording context.
// Neither path can invoke a native motor API, including existing GameLevel
// tests that do not explicitly know about rumble.
namespace painter_rumble
{
    struct Recording
    {
        struct Call { int slot = -1; RumbleMotors motors; };
        std::array<Call, 4096> calls{};
        std::array<RumbleMotors, input_slots::pad_count> current{};
        std::size_t count = 0;
        std::size_t factories = 0;
        std::size_t destructions = 0;
        bool invalid = false;
    };

    // Test-only factory context. A leaf clears its StateContext before this
    // scope and its Recording die, so constructed sinks cannot dangle.
    inline Recording* recording = nullptr;
    class RecordScope final
    {
    public:
        explicit RecordScope(Recording& target) noexcept : previous_(recording) { recording = &target; }
        ~RecordScope() noexcept { recording = this->previous_; }
        RecordScope(const RecordScope&) = delete;
        RecordScope& operator=(const RecordScope&) = delete;
    private:
        Recording* previous_;
    };

    class Sink final : public RumbleSink
    {
    public:
        explicit Sink(Recording* target) noexcept : target_(target)
        {
            if (this->target_ != nullptr) { ++this->target_->factories; }
        }
        ~Sink() noexcept override
        {
            if (this->target_ != nullptr) { ++this->target_->destructions; }
        }
        void set_vibration(int slot, const RumbleMotors& motors) noexcept override
        {
            if (this->target_ == nullptr) { return; }
            Recording& target = *this->target_;
            if (!input_slots::is_pad(slot) || target.count >= target.calls.size() ||
                !std::isfinite(motors.low_frequency) || !std::isfinite(motors.high_frequency) ||
                motors.low_frequency < 0.0f || motors.low_frequency > 1.0f ||
                motors.high_frequency < 0.0f || motors.high_frequency > 1.0f)
            {
                target.invalid = true;
                return;
            }
            target.calls[target.count++] = { slot, motors };
            target.current[static_cast<std::size_t>(slot)] = motors;
        }
    private:
        Recording* target_;
    };
}

std::unique_ptr<RumbleSink> make_rumble_sink()
{
    return std::make_unique<painter_rumble::Sink>(painter_rumble::recording);
}
