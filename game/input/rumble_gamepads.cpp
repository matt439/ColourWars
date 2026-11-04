#include "game/pch.h"
#include "game/input/rumble_sink.h"
#include "engine/input/gamepads.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
    static_assert(input_slots::pad_count == labrador::Gamepads::max_count);
    static_assert(maximum_rumble_pulse_seconds <= labrador::max_vibration_seconds);

    bool normalized(float value) noexcept
    {
        return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
    }

    class GamepadsRumbleSink final : public RumbleSink
    {
    public:
        explicit GamepadsRumbleSink(labrador::Gamepads& gamepads) noexcept : gamepads_(gamepads) {}

        ~GamepadsRumbleSink() noexcept override
        {
            for (std::size_t slot = 0; slot < this->active_.size(); ++slot)
            {
                if (this->active_[slot]) { this->stop(static_cast<int>(slot)); }
            }
        }

        void set_vibration(int pad_slot, const RumbleMotors& motors, float seconds) noexcept override
        {
            if (!input_slots::is_pad(pad_slot)) { return; }
            const bool audible = normalized(motors.low_frequency) && normalized(motors.high_frequency) &&
                (motors.low_frequency > 0.0f || motors.high_frequency > 0.0f) &&
                std::isfinite(seconds) && seconds > 0.0f;
            // Track intent, not a promise about hardware state. An unavailable
            // device never triggers retries or suppresses a later new request.
            this->active_[static_cast<std::size_t>(pad_slot)] = audible;
            if (!audible)
            {
                this->stop(pad_slot);
                return;
            }
            // Checked here so check_vibration has nothing to refuse.
            const labrador::Vibration request{ motors.low_frequency, motors.high_frequency,
                (std::min)(seconds, labrador::max_vibration_seconds) };
            try { (void)this->gamepads_.vibrate(pad_slot, request); }
            catch (...) {}
        }

    private:
        void stop(int pad_slot) noexcept
        {
            try { this->gamepads_.stop_vibration(pad_slot); }
            catch (...) {}
        }

        labrador::Gamepads& gamepads_;
        std::array<bool, input_slots::pad_count> active_{};
    };
}

std::unique_ptr<RumbleSink> make_rumble_sink(labrador::Gamepads& gamepads)
{
    return std::make_unique<GamepadsRumbleSink>(gamepads);
}
