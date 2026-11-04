#include "game/pch.h"
#include "game/input/rumble_sink.h"
#include "engine/input/gamepad_reader.h"
#include "engine/input/gamepads.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

// The concrete reader link seam: the real adapter drives the real
// labrador::Gamepads, and this reader stands where the pad API would. Nothing
// here reaches XInput or a motor.
namespace rumble_adapter_pads
{
    struct Call
    {
        bool stop = false;
        int slot = -1;
        labrador::Vibration vibration;
    };

    struct Trace
    {
        std::array<Call, 64> calls{};
        std::size_t count = 0;
        bool overflow = false;
        bool accepts = true;
        bool throws = false;

        void record(const Call& call)
        {
            if (this->count >= this->calls.size()) { this->overflow = true; return; }
            this->calls[this->count++] = call;
        }
    };

    std::array<labrador::GamepadState, 4> states{};
    Trace* trace = nullptr;
}

namespace labrador
{
    struct GamepadReader::Impl { std::array<GamepadState, 4> states{}; };
    GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
    GamepadReader::~GamepadReader() = default;
    GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
    GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
    void GamepadReader::sample() { this->impl_->states = rumble_adapter_pads::states; }
    GamepadState GamepadReader::read(int slot) const
    {
        return slot >= 0 && slot < 4 ? this->impl_->states[static_cast<size_t>(slot)] : GamepadState{};
    }
    void GamepadReader::suspend() {}
    void GamepadReader::resume() {}
    bool GamepadReader::vibrate(int slot, const Vibration& vibration)
    {
        rumble_adapter_pads::Trace& trace = *rumble_adapter_pads::trace;
        trace.record({ false, slot, vibration });
        if (trace.throws) { throw std::runtime_error("fixture pad API failure"); }
        return trace.accepts;
    }
    void GamepadReader::stop_vibration(int slot)
    {
        rumble_adapter_pads::Trace& trace = *rumble_adapter_pads::trace;
        trace.record({ true, slot, {} });
        if (trace.throws) { throw std::runtime_error("fixture pad API failure"); }
    }
}

namespace
{
    using acceptance::require;
    using rumble_adapter_pads::Trace;

    // One reader and Gamepads per check, polled once with the chosen pads.
    class Pads
    {
    public:
        explicit Pads(Trace& trace, std::array<bool, 4> connected = { true, true, true, true }) :
            gamepads_(&reader_)
        {
            rumble_adapter_pads::trace = &trace;
            for (std::size_t slot = 0; slot < connected.size(); ++slot)
            {
                rumble_adapter_pads::states[slot] = {};
                rumble_adapter_pads::states[slot].connected = connected[slot];
            }
            this->gamepads_.poll();
        }
        labrador::Gamepads& gamepads() { return this->gamepads_; }
    private:
        labrador::GamepadReader reader_;
        labrador::Gamepads gamepads_;
    };

    void vibrated(const Trace& trace, std::size_t index, int slot, float low, float high, float seconds)
    {
        require(!trace.overflow && index < trace.count && !trace.calls[index].stop &&
            trace.calls[index].slot == slot &&
            trace.calls[index].vibration.low_frequency == low &&
            trace.calls[index].vibration.high_frequency == high &&
            trace.calls[index].vibration.seconds == seconds,
            "Rumble adapter vibration trace differs");
    }

    void stopped(const Trace& trace, std::size_t index, int slot)
    {
        require(!trace.overflow && index < trace.count && trace.calls[index].stop &&
            trace.calls[index].slot == slot, "Rumble adapter stop trace differs");
    }

    void requests_and_slots()
    {
        Trace trace;
        {
            Pads pads(trace);
            auto sink = make_rumble_sink(pads.gamepads());
            require(trace.count == 0, "Constructing a rumble sink contacted the pads");
            for (const int slot : { -1, input_slots::keyboard_and_mouse, input_slots::count,
                (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
            {
                sink->set_vibration(slot, { 1.0f, 1.0f }, 0.5f);
                sink->set_vibration(slot, {}, 0.0f);
            }
            require(trace.count == 0, "Invalid/keyboard slot reached the engine's pads");
            sink->set_vibration(3, { 1.0f, 0.0f }, 0.5f);
            vibrated(trace, 0, 3, 1.0f, 0.0f, 0.5f);
            sink->set_vibration(1, { 0.5f, 0.25f }, 0.125f);
            vibrated(trace, 1, 1, 0.5f, 0.25f, 0.125f);
            // Past the engine's bound is clamped rather than refused.
            sink->set_vibration(1, { 0.5f, 0.25f }, labrador::max_vibration_seconds * 2.0f);
            vibrated(trace, 2, 1, 0.5f, 0.25f, labrador::max_vibration_seconds);
            sink->set_vibration(1, {}, 0.0f);
            stopped(trace, 3, 1);
        }
        require(trace.count == 5, "Sink destruction stopped untouched/already-stopped slots or omitted active output");
        stopped(trace, 4, 3);
    }

    void invalid_requests_stop()
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();
        for (const float invalid : { -0.001f, 1.001f, nan, infinity, -infinity })
        {
            for (const bool invalid_low : { false, true })
            {
                Trace trace;
                {
                    Pads pads(trace);
                    auto sink = make_rumble_sink(pads.gamepads());
                    sink->set_vibration(2, { 1.0f, 1.0f }, 0.5f);
                    sink->set_vibration(2, invalid_low ? RumbleMotors{ invalid, 1.0f } : RumbleMotors{ 1.0f, invalid }, 0.5f);
                    stopped(trace, 1, 2);
                }
                require(trace.count == 2, "Invalid motor input left stale output or caused a retry");
            }
        }
        for (const float invalid : { 0.0f, -0.0f, -0.5f, nan, infinity, -infinity })
        {
            Trace trace;
            {
                Pads pads(trace);
                auto sink = make_rumble_sink(pads.gamepads());
                sink->set_vibration(0, { 1.0f, 1.0f }, 0.5f);
                sink->set_vibration(0, { 1.0f, 1.0f }, invalid);
                stopped(trace, 1, 0);
            }
            require(trace.count == 2, "An unbounded or empty deadline reached the engine or left stale output");
        }
    }

    void unavailable_and_unwind()
    {
        Trace refused;
        refused.accepts = false;
        {
            Pads pads(refused);
            auto sink = make_rumble_sink(pads.gamepads());
            sink->set_vibration(3, { 0.5f, 0.0f }, 0.25f);
            require(refused.count == 1, "Refused request entered a retry loop");
            refused.accepts = true;
            sink->set_vibration(3, { 0.0f, 0.5f }, 0.25f);
            vibrated(refused, 1, 3, 0.0f, 0.5f, 0.25f);
        }
        require(refused.count == 3, "A refused request suppressed a later request or lifetime stop");
        stopped(refused, 2, 3);

        // An empty slot is answered by Gamepads without the reader; the
        // intent is still stopped at destruction, which is safe on any slot.
        Trace absent;
        {
            Pads pads(absent, { true, false, true, true });
            auto sink = make_rumble_sink(pads.gamepads());
            sink->set_vibration(1, { 1.0f, 1.0f }, 0.5f);
            require(absent.count == 0, "A request for an empty slot reached the reader");
        }
        require(absent.count == 1, "Destruction did not stop an empty slot's outstanding intent");
        stopped(absent, 0, 1);

        // Nothing the pads throw leaves the nonthrowing sink.
        Trace throwing;
        throwing.throws = true;
        {
            Pads pads(throwing);
            auto sink = make_rumble_sink(pads.gamepads());
            sink->set_vibration(0, { 1.0f, 1.0f }, 0.5f);
            sink->set_vibration(2, { 1.0f, 1.0f }, 0.5f);
            sink->set_vibration(2, {}, 0.0f);
        }
        require(throwing.count == 4, "A throwing pad API was retried or skipped a stop");
        vibrated(throwing, 0, 0, 1.0f, 1.0f, 0.5f);
        stopped(throwing, 2, 2);
        stopped(throwing, 3, 0);

        struct ExpectedFailure {};
        Trace unwind;
        bool caught = false;
        {
            Pads pads(unwind);
            try
            {
                auto sink = make_rumble_sink(pads.gamepads());
                sink->set_vibration(1, { 1.0f, 0.0f }, 0.5f);
                throw ExpectedFailure{};
            }
            catch (const ExpectedFailure&) { caught = true; }
        }
        require(caught && unwind.count == 2, "Sink did not stop through an owning exception unwind");
        stopped(unwind, 1, 1);
    }

    void policy_stop_path()
    {
        Trace trace;
        {
            Pads pads(trace);
            auto sink = make_rumble_sink(pads.gamepads());
            RumbleProfile profile{};
            profile[static_cast<std::size_t>(RumbleEvent::hit)] = { { 1.0f, 0.5f }, 0.125f };
            {
                Rumble rumble(*sink, profile);
                rumble.set_session({ true, true, false });
                rumble.set_slot(3, { true, true, false, false });
                rumble.trigger(3, RumbleEvent::hit);
                vibrated(trace, 0, 3, 1.0f, 0.5f, 0.125f);
                rumble.update(0.0625f);
                rumble.trigger(3, RumbleEvent::hit);
                vibrated(trace, 1, 3, 1.0f, 0.5f, 0.125f);
                rumble.update(0.125f);
                stopped(trace, 2, 3);
                rumble.trigger(3, RumbleEvent::hit);
                vibrated(trace, 3, 3, 1.0f, 0.5f, 0.125f);
            }
            for (int slot = 0; slot < input_slots::pad_count; ++slot)
            {
                stopped(trace, 4 + static_cast<std::size_t>(slot), slot);
            }
        }
        require(trace.count == 8, "Adapter retained nonzero intent after the policy destructor stopped all pads");
    }
}

int main()
{
    return acceptance::run_probe("rumble adapter", [&]
    {
        requests_and_slots();
        invalid_requests_stop();
        unavailable_and_unwind();
        policy_stop_path();
        rumble_adapter_pads::trace = nullptr;
        std::cout << "PASS rumble adapter: bounded Gamepads::vibrate requests and stop_vibration stops through "
            "the real labrador::Gamepads, physical slot bounds, invalid-value and invalid-deadline stop, "
            "clamped engine bound, refused/empty/throwing pads, lifetime unwind and policy renew/stop path. "
            "Fake GamepadReader only; no XInput request, physical pad or feel acceptance.\n";
        return 0;
    });
}
