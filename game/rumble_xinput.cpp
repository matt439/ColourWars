#include "game/pch.h"
#include "game/rumble_sink.h"
#include <Xinput.h>
#include <array>
#include <cmath>
#include <limits>

namespace
{
    bool normalized(float value) noexcept
    {
        return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
    }

    std::uint16_t motor_speed(float value) noexcept
    {
        return static_cast<std::uint16_t>(std::lround(static_cast<double>(value) *
            static_cast<double>((std::numeric_limits<std::uint16_t>::max)())));
    }

    bool request_xinput(void* /*context*/, int pad_slot,
        std::uint16_t low_frequency, std::uint16_t high_frequency) noexcept
    {
        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = low_frequency;
        vibration.wRightMotorSpeed = high_frequency;
        return ::XInputSetState(static_cast<DWORD>(pad_slot), &vibration) == ERROR_SUCCESS;
    }

    class XInputRumbleSink final : public RumbleSink
    {
    public:
        XInputRumbleSink(RumbleMotorRequest& request, void* context) noexcept :
            request_(request), context_(context) {}

        ~XInputRumbleSink() noexcept override
        {
            for (std::size_t slot = 0; slot < this->active_.size(); ++slot)
            {
                if (this->active_[slot])
                {
                    (void)this->request_(this->context_, static_cast<int>(slot), 0, 0);
                }
            }
        }

        void set_vibration(int pad_slot, const RumbleMotors& motors) noexcept override
        {
            if (!input_slots::is_pad(pad_slot)) { return; }
            std::uint16_t low = 0;
            std::uint16_t high = 0;
            if (normalized(motors.low_frequency) && normalized(motors.high_frequency))
            {
                low = motor_speed(motors.low_frequency);
                high = motor_speed(motors.high_frequency);
            }
            // Track intent, not a promise about hardware state. An unavailable
            // device never triggers retries or suppresses a later new request.
            this->active_[static_cast<std::size_t>(pad_slot)] = low != 0 || high != 0;
            (void)this->request_(this->context_, pad_slot, low, high);
        }

    private:
        RumbleMotorRequest& request_;
        void* context_;
        std::array<bool, input_slots::pad_count> active_{};
    };
}

std::unique_ptr<RumbleSink> make_rumble_sink()
{
    return make_rumble_sink(request_xinput, nullptr);
}

std::unique_ptr<RumbleSink> make_rumble_sink(RumbleMotorRequest& request, void* context)
{
    return std::make_unique<XInputRumbleSink>(request, context);
}
