#include "game/pch.h"
#include "game/rumble_sink.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    struct Requests
    {
        struct Call
        {
            int slot = -1;
            std::uint16_t low = 0;
            std::uint16_t high = 0;
        };
        std::array<Call, 64> calls{};
        std::size_t count = 0;
        bool succeeds = true;
        bool overflow = false;

        void exact(std::size_t index, int slot, std::uint16_t low, std::uint16_t high) const
        {
            require(!overflow && index < count && calls[index].slot == slot &&
                calls[index].low == low && calls[index].high == high,
                "Rumble adapter request trace differs");
        }
    };

    bool record(void* context, int slot, std::uint16_t low, std::uint16_t high) noexcept
    {
        auto& requests = *static_cast<Requests*>(context);
        if (requests.count >= requests.calls.size()) { requests.overflow = true; return false; }
        requests.calls[requests.count++] = { slot, low, high };
        return requests.succeeds;
    }

    void conversion_and_slots()
    {
        Requests requests;
        {
            auto sink = make_rumble_sink(record, &requests);
            require(requests.count == 0, "Constructing a rumble sink contacted the transport");
            for (const int slot : { -1, input_slots::keyboard_and_mouse, input_slots::count,
                (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
            {
                sink->set_vibration(slot, { 1.0f, 1.0f });
            }
            require(requests.count == 0, "Invalid/keyboard slot reached native rumble transport");
            sink->set_vibration(3, { 1.0f, 0.0f });
            requests.exact(0, 3, 65535, 0);
            sink->set_vibration(1, { 0.5f, 0.25f });
            requests.exact(1, 1, 32768, 16384);
            sink->set_vibration(1, { 1.0f / 65535.0f, 0.0000001f });
            requests.exact(2, 1, 1, 0);
            sink->set_vibration(1, {});
            requests.exact(3, 1, 0, 0);
        }
        require(requests.count == 5, "Sink destruction stopped untouched/already-stopped slots or omitted active output");
        requests.exact(4, 3, 0, 0);
    }

    void invalid_motors_stop()
    {
        for (const float invalid : { -0.001f, 1.001f, std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() })
        {
            for (const bool invalid_low : { false, true })
            {
                Requests requests;
                {
                    auto sink = make_rumble_sink(record, &requests);
                    sink->set_vibration(2, { 1.0f, 1.0f });
                    sink->set_vibration(2, invalid_low ? RumbleMotors{ invalid, 1.0f } : RumbleMotors{ 1.0f, invalid });
                    requests.exact(1, 2, 0, 0);
                }
                require(requests.count == 2, "Invalid motor input left stale output or caused a retry");
            }
        }
    }

    void unavailable_and_unwind()
    {
        Requests requests;
        requests.succeeds = false;
        {
            auto sink = make_rumble_sink(record, &requests);
            sink->set_vibration(3, { 0.5f, 0.0f });
            require(requests.count == 1, "Failed native request entered a retry loop");
            requests.succeeds = true;
            sink->set_vibration(3, { 0.0f, 0.5f });
            requests.exact(1, 3, 0, 32768);
        }
        require(requests.count == 3, "A failed native request suppressed a later request or lifetime stop");
        requests.exact(2, 3, 0, 0);

        Requests failed_stop;
        failed_stop.succeeds = false;
        {
            auto sink = make_rumble_sink(record, &failed_stop);
            sink->set_vibration(0, { 1.0f, 1.0f });
            sink->set_vibration(0, {});
        }
        require(failed_stop.count == 2, "Failed explicit stop was retried on destruction");
        failed_stop.exact(1, 0, 0, 0);

        struct ExpectedFailure {};
        Requests unwind;
        bool caught = false;
        try
        {
            auto sink = make_rumble_sink(record, &unwind);
            sink->set_vibration(1, { 1.0f, 0.0f });
            throw ExpectedFailure{};
        }
        catch (const ExpectedFailure&) { caught = true; }
        require(caught && unwind.count == 2, "Sink did not stop through an owning exception unwind");
        unwind.exact(1, 1, 0, 0);
    }

    void policy_stop_path()
    {
        Requests requests;
        {
            auto sink = make_rumble_sink(record, &requests);
            RumbleProfile profile{};
            profile[static_cast<std::size_t>(RumbleEvent::hit)] = { { 1.0f, 0.5f }, 0.125f };
            {
                Rumble rumble(*sink, profile);
                rumble.set_session({ true, true, false });
                rumble.set_slot(3, { true, true, false, false });
                rumble.trigger(3, RumbleEvent::hit);
                requests.exact(0, 3, 65535, 32768);
                rumble.update(0.125f);
                requests.exact(1, 3, 0, 0);
                rumble.trigger(3, RumbleEvent::hit);
                requests.exact(2, 3, 65535, 32768);
            }
            for (int slot = 0; slot < input_slots::pad_count; ++slot)
            {
                requests.exact(3 + static_cast<std::size_t>(slot), slot, 0, 0);
            }
        }
        require(requests.count == 7, "Native adapter retained nonzero intent after the policy destructor stopped all pads");
    }
}

int main()
{
    try
    {
        conversion_and_slots();
        invalid_motors_stop();
        unavailable_and_unwind();
        policy_stop_path();
        std::cout << "PASS rumble adapter: normalized 16-bit intent, physical slot bounds, invalid-value stop, "
            "unavailable/failing transport, lifetime unwind and policy stop path. Injected callback only; "
            "no default factory, XInput request, physical pad or feel acceptance.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL rumble adapter: " << error.what() << '\n';
        return 1;
    }
}
