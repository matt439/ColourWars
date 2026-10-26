#include "game/pch.h"
#include "game/rumble.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <filesystem>

namespace rumble_acceptance { void check_callers(const std::filesystem::path& content); }

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    bool equals(const RumbleMotors& actual, float low, float high)
    {
        return actual.low_frequency == low && actual.high_frequency == high;
    }

    class RecordingSink final : public RumbleSink
    {
    public:
        struct Call { int slot = -1; RumbleMotors motors; };
        std::array<Call, 256> calls{};
        std::array<RumbleMotors, input_slots::pad_count> current{};
        std::size_t count = 0;
        bool invalid = false;

        void set_vibration(int slot, const RumbleMotors& motors) noexcept override
        {
            if (!input_slots::is_pad(slot) || count >= calls.size() ||
                !std::isfinite(motors.low_frequency) || !std::isfinite(motors.high_frequency) ||
                motors.low_frequency < 0.0f || motors.low_frequency > 1.0f ||
                motors.high_frequency < 0.0f || motors.high_frequency > 1.0f)
            {
                invalid = true;
                return;
            }
            calls[count++] = { slot, motors };
            current[static_cast<std::size_t>(slot)] = motors;
        }

        void exact(std::size_t index, int slot, float low, float high) const
        {
            require(!invalid && index < count && calls[index].slot == slot &&
                equals(calls[index].motors, low, high), "Rumble sink trace differs");
        }

        void all_zero() const
        {
            require(!invalid, "Rumble emitted an invalid slot/amplitude or overflowed the fake sink");
            for (const RumbleMotors& motors : current)
            {
                require(equals(motors, 0.0f, 0.0f), "A rumble motor remains active");
            }
        }
    };

    // Exactly representable test pulses, not shipped cue tuning or feel claims.
    RumbleProfile profile()
    {
        return { RumblePulse{ { 0.75f, 0.25f }, 0.25f },
            RumblePulse{ { 1.0f, 1.0f }, 0.5f },
            RumblePulse{ { 0.5f, 0.0f }, 0.125f },
            RumblePulse{ { 0.0f, 0.5f }, 0.0625f } };
    }

    constexpr RumbleSlotState live_slot{ true, true, false, false };
    constexpr RumbleSessionState live_session{ true, true, false };

    void enable(Rumble& rumble)
    {
        rumble.set_session(live_session);
        for (int slot = 0; slot < input_slots::pad_count; ++slot)
        {
            rumble.set_slot(slot, live_slot);
        }
    }

    template<class Exception, class Action>
    void rejects(Action action, const char* message)
    {
        bool rejected = false;
        try { action(); }
        catch (const Exception&) { rejected = true; }
        require(rejected, message);
    }

    void events_and_expiry()
    {
        const RumbleProfile pulses = profile();
        RecordingSink sink;
        {
            Rumble rumble(sink, pulses);
            rumble.set_slot(3, live_slot);
            rumble.trigger(3, RumbleEvent::hit);
            rumble.update(0.0f);
            require(sink.count == 0, "Inactive match emitted a pulse");
            rumble.set_session({ true, false, false });
            rumble.trigger(3, RumbleEvent::elimination);
            require(sink.count == 0, "Unfocused match emitted a pulse");
            rumble.set_session(live_session);

            for (std::size_t event = 0; event < rumble_event_count; ++event)
            {
                const std::size_t before = sink.count;
                const RumblePulse& pulse = pulses[event];
                rumble.trigger(3, static_cast<RumbleEvent>(event));
                sink.exact(before, 3, pulse.motors.low_frequency, pulse.motors.high_frequency);
                rumble.update(0.0f);
                rumble.update(pulse.duration_seconds / 2.0f);
                require(sink.count == before + 1, "An unexpired pulse changed or resent its motor output");
                rumble.update(pulse.duration_seconds / 2.0f);
                sink.exact(before + 1, 3, 0.0f, 0.0f);
                require(sink.count == before + 2, "Pulse expiry sent extra output");
            }
        }
        require(sink.count == 12, "Destruction did not stop exactly the four physical pads");
        for (int slot = 0; slot < input_slots::pad_count; ++slot)
        {
            sink.exact(8 + static_cast<std::size_t>(slot), slot, 0.0f, 0.0f);
        }
        sink.all_zero();
    }

    void overlap_and_retrigger()
    {
        RecordingSink sink;
        Rumble rumble(sink, profile());
        enable(rumble);
        rumble.trigger(2, RumbleEvent::hit);
        rumble.trigger(2, RumbleEvent::dry_fire);
        sink.exact(0, 2, 0.75f, 0.25f);
        sink.exact(1, 2, 0.75f, 0.5f);
        rumble.update(0.03125f);
        rumble.trigger(2, RumbleEvent::dry_fire);
        rumble.update(0.03125f);
        require(sink.count == 2, "Retriggered dry-fire pulse expired early or added motor amplitudes");
        rumble.update(0.03125f);
        sink.exact(2, 2, 0.75f, 0.25f);
        rumble.update(0.15625f);
        sink.exact(3, 2, 0.0f, 0.0f);
        require(sink.count == 4, "Repeating one event extended an unrelated event");

        rumble.trigger(2, RumbleEvent::hit);
        rumble.update(0.125f);
        rumble.trigger(2, RumbleEvent::hit);
        rumble.update(0.125f);
        require(sink.count == 5 && equals(sink.current[2], 0.75f, 0.25f),
            "Repeated hit did not refresh its own pulse without resending unchanged output");
        rumble.update(0.125f);
        sink.exact(5, 2, 0.0f, 0.0f);

        rumble.trigger(2, RumbleEvent::hit);
        rumble.trigger(2, RumbleEvent::elimination);
        rumble.trigger(2, RumbleEvent::hard_landing);
        rumble.trigger(2, RumbleEvent::dry_fire);
        sink.exact(7, 2, 1.0f, 1.0f);
        rumble.update(0.25f);
        require(sink.count == 8, "Weaker overlapping cues shortened or altered an elimination pulse");
        rumble.update(0.25f);
        sink.exact(8, 2, 0.0f, 0.0f);
        require(sink.count == 9, "Overlap generated extra writes");
    }

    void sparse_slots_and_keyboard()
    {
        RecordingSink sink;
        Rumble rumble(sink, profile());
        rumble.set_session(live_session);
        rumble.set_slot(3, live_slot);
        rumble.set_slot(1, live_slot);
        rumble.trigger(3, RumbleEvent::hard_landing);
        rumble.trigger(1, RumbleEvent::dry_fire);
        sink.exact(0, 3, 0.5f, 0.0f);
        sink.exact(1, 1, 0.0f, 0.5f);
        for (const int slot : { 0, 2, input_slots::keyboard_and_mouse })
        {
            rumble.trigger(slot, RumbleEvent::elimination);
        }
        rumble.set_slot(input_slots::keyboard_and_mouse, live_slot);
        require(sink.count == 2, "Keyboard or disconnected slots received a pulse");
        require(equals(sink.current[0], 0.0f, 0.0f) && equals(sink.current[2], 0.0f, 0.0f),
            "Sparse roster was treated as consecutive device slots");
        rumble.update(0.0625f);
        sink.exact(2, 1, 0.0f, 0.0f);
        require(equals(sink.current[3], 0.5f, 0.0f), "One pad's expiry stopped a different pad");
        rumble.update(0.0625f);
        sink.exact(3, 3, 0.0f, 0.0f);
    }

    void slot_gates()
    {
        const std::array<RumbleSlotState, 4> forbidden{{
            { false, true, false, false }, { true, false, false, false },
            { true, true, true, false }, { true, true, false, true } }};
        for (const RumbleSlotState& blocked : forbidden)
        {
            RecordingSink sink;
            Rumble rumble(sink, profile());
            enable(rumble);
            rumble.trigger(3, RumbleEvent::elimination);
            rumble.trigger(1, RumbleEvent::hit);
            rumble.set_slot(3, blocked);
            sink.exact(2, 3, 0.0f, 0.0f);
            require(equals(sink.current[1], 0.75f, 0.25f), "Slot gate stopped a different physical pad");
            for (std::size_t event = 0; event < rumble_event_count; ++event)
            {
                rumble.trigger(3, static_cast<RumbleEvent>(event));
            }
            rumble.set_slot(3, blocked);
            require(sink.count == 3, "Disabled, bot, replay or disconnected slot emitted new output");
            rumble.set_slot(3, live_slot);
            rumble.update(0.0f);
            require(sink.count == 3, "Slot reconnect/re-enable resumed stale pulses");
            rumble.trigger(3, RumbleEvent::dry_fire);
            sink.exact(3, 3, 0.0f, 0.5f);
        }
    }

    void session_gates_and_stops()
    {
        const std::array<RumbleSessionState, 3> forbidden{{
            { true, true, true }, { true, false, false }, { false, true, false } }};
        for (const RumbleSessionState& blocked : forbidden)
        {
            RecordingSink sink;
            Rumble rumble(sink, profile());
            enable(rumble);
            for (int slot = 0; slot < input_slots::pad_count; ++slot)
            {
                rumble.trigger(slot, RumbleEvent::elimination);
            }
            rumble.set_session(blocked);
            for (int slot = 0; slot < input_slots::pad_count; ++slot)
            {
                sink.exact(4 + static_cast<std::size_t>(slot), slot, 0.0f, 0.0f);
                rumble.trigger(slot, RumbleEvent::hit);
            }
            rumble.set_session(blocked);
            rumble.update(0.0f);
            require(sink.count == 8, "Paused, unfocused or ended match emitted or repeatedly stopped output");
            rumble.set_session(live_session);
            rumble.update(0.0f);
            require(sink.count == 8, "Session resumed stale pulses after a stop condition");
            rumble.trigger(1, RumbleEvent::hit);
            sink.exact(8, 1, 0.75f, 0.25f);
            rumble.stop_all();
            sink.all_zero();
            rumble.update(0.0f);
            require(sink.count == 13, "Explicit stop did not clear all pending pulses");
        }

        RecordingSink sink;
        {
            Rumble rumble(sink, profile());
            enable(rumble);
            rumble.trigger(3, RumbleEvent::elimination);
        }
        require(sink.count == 5, "Live-policy destruction did not request every pad stop");
        sink.all_zero();
    }

    void invalid_inputs_are_atomic()
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();
        for (const float invalid : { -0.125f, 1.125f, nan, infinity, -infinity })
        {
            for (int field = 0; field < 3; ++field)
            {
                RecordingSink sink;
                RumbleProfile malformed = profile();
                RumblePulse& pulse = malformed.back();
                if (field == 0) { pulse.motors.low_frequency = invalid; }
                if (field == 1) { pulse.motors.high_frequency = invalid; }
                if (field == 2) { pulse.duration_seconds = invalid; }
                rejects<std::invalid_argument>([&] { Rumble rejected(sink, malformed); },
                    "Invalid rumble profile was accepted");
                require(sink.count == 0 && !sink.invalid, "Profile rejection touched the motor sink");
            }
        }

        RecordingSink sink;
        Rumble rumble(sink, profile());
        enable(rumble);
        rumble.trigger(1, RumbleEvent::hit);
        rumble.trigger(3, RumbleEvent::hit);
        for (const float invalid : { -0.125f, nan, infinity, -infinity })
        {
            rejects<std::invalid_argument>([&] { rumble.update(invalid); }, "Invalid rumble duration was accepted");
        }
        for (const int invalid : { -1, input_slots::count, std::numeric_limits<int>::max() })
        {
            rejects<std::out_of_range>([&] { rumble.set_slot(invalid, live_slot); }, "Invalid rumble state slot was accepted");
            rejects<std::out_of_range>([&] { rumble.trigger(invalid, RumbleEvent::hit); }, "Invalid rumble event slot was accepted");
        }
        for (const RumbleEvent invalid : { static_cast<RumbleEvent>(-1), RumbleEvent::count })
        {
            rejects<std::out_of_range>([&] { rumble.trigger(1, invalid); }, "Invalid rumble event was accepted");
            rejects<std::out_of_range>([&] { rumble.trigger(input_slots::keyboard_and_mouse, invalid); },
                "Keyboard slot bypassed event validation");
        }
        require(sink.count == 2 && equals(sink.current[1], 0.75f, 0.25f) && equals(sink.current[3], 0.75f, 0.25f),
            "Invalid request changed rumble output");
        rumble.update(0.25f);
        require(sink.count == 4, "Invalid request changed pending pulse lifetime");
        sink.all_zero();
    }

    void silent_and_boundary_profiles()
    {
        RecordingSink sink;
        {
            RumbleProfile silent{};
            silent[0] = { { 1.0f, 1.0f }, 0.0f };
            silent[1] = { { 0.0f, 0.0f }, maximum_rumble_pulse_seconds };
            Rumble rumble(sink, silent);
            enable(rumble);
            for (std::size_t event = 0; event < rumble_event_count; ++event)
            {
                rumble.trigger(0, static_cast<RumbleEvent>(event));
            }
            rumble.update(-0.0f);
            require(sink.count == 0, "Silent or zero-duration event generated a motor command");
        }
        {
            RumbleProfile boundary{};
            boundary[0] = { { 1.0f, 1.0f }, maximum_rumble_pulse_seconds };
            Rumble rumble(sink, boundary);
            enable(rumble);
            rumble.trigger(0, RumbleEvent::hit);
            sink.exact(4, 0, 1.0f, 1.0f);
            rumble.update(std::numeric_limits<float>::max());
            sink.exact(5, 0, 0.0f, 0.0f);
        }
        sink.all_zero();
    }

    static_assert(std::is_nothrow_destructible_v<Rumble>);
    static_assert(!std::is_copy_constructible_v<Rumble> && !std::is_move_constructible_v<Rumble>);
}

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        require(argc == 2, "rumble <content>");
        events_and_expiry();
        overlap_and_retrigger();
        sparse_slots_and_keyboard();
        slot_gates();
        session_gates_and_stops();
        invalid_inputs_are_atomic();
        silent_and_boundary_profiles();
        rumble_acceptance::check_callers(std::filesystem::absolute(argv[1]));
        std::cout << "PASS rumble pulse policy: four event intents, independent expiry/retrigger and bounded overlap, "
            "sparse physical slots, keyboard/bot/replay/disabled/disconnected gates, pause/focus/match/destructor stops, "
            "no stale resume and atomic invalid-input rejection. Fake sink only; no native motors, "
            "GameLevel output or physical feel acceptance.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL rumble pulse policy: " << error.what() << '\n';
        return 1;
    }
}
