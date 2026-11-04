#pragma once

#include "game/input/input_slots.h"
#include <array>
#include <cstddef>

enum class RumbleEvent
{
    hit,
    elimination,
    hard_landing,
    dry_fire,
    count
};

inline constexpr std::size_t rumble_event_count = static_cast<std::size_t>(RumbleEvent::count);

struct RumbleMotors
{
    float low_frequency = 0.0f;
    float high_frequency = 0.0f;
};

struct RumblePulse
{
    RumbleMotors motors;
    // Zero disables this event. Individual pulses cannot outlive this bound;
    // the four actual cue values belong to authored tuning, not this policy.
    float duration_seconds = 0.0f;
};

inline constexpr float maximum_rumble_pulse_seconds = 1.0f;
using RumbleProfile = std::array<RumblePulse, rumble_event_count>;

struct RumbleSlotState
{
    bool connected = false;
    bool enabled = true;
    bool bot = false;
    bool replay = false;
};

struct RumbleSessionState
{
    bool match_active = false;
    bool focused = false;
    bool paused = false;
};

// The sink accepts normalized motor intent and must remain alive until its
// owning Rumble is destroyed. Native/device failure handling belongs to the
// adapter; a policy stop makes a best-effort zero request, not a success claim.
// Nonzero motors carry `seconds`, the longest the request may run unrenewed, in
// (0, maximum_rumble_pulse_seconds]; the engine stops a request whose seconds
// pass. All-zero motors are a stop and their seconds are 0.
class RumbleSink
{
public:
    virtual ~RumbleSink() = default;
    virtual void set_vibration(int pad_slot, const RumbleMotors& motors, float seconds) noexcept = 0;
};

// One match owns all physical pad slots. Slots are device indices, never roster
// indices. Keyboard events are accepted but ignored. No native API is used here.
// Each event has an independent timer; overlaps take the maximum per motor,
// and repeating an event refreshes only its own timer. Output changes are sent
// immediately; update advances timers. Gate loss cancels rather than freezes.
// Every nonzero request carries the time until the output would fall to zero
// (the longest remaining nonzero pulse), so a match that stops updating still
// ends at the engine's deadline. Every audible trigger sends the current output
// again with that time; between triggers an unchanged output is resent only
// when the time outgrows what the last request was given. The engine's deadline
// runs on wall time and these timers on game time (update's dt), so a stall the
// step timer clamps can stop the pad before this policy's pulse ends; the next
// audible trigger restarts it.
class Rumble final
{
public:
    Rumble(RumbleSink& sink, const RumbleProfile& profile);
    ~Rumble() noexcept;

    Rumble(const Rumble&) = delete;
    Rumble& operator=(const Rumble&) = delete;
    Rumble(Rumble&&) = delete;
    Rumble& operator=(Rumble&&) = delete;

    void set_session(const RumbleSessionState& state) noexcept;
    void set_slot(int input_slot, const RumbleSlotState& state);
    void trigger(int input_slot, RumbleEvent event);
    void update(float dt);

    // Cancels all pending pulses and requests zero on every physical pad. It
    // does not change the gates: a later permitted event may start a new pulse.
    void stop_all() noexcept;

private:
    struct Slot
    {
        RumbleSlotState state;
        std::array<double, rumble_event_count> remaining{};
        RumbleMotors output;
        // What remains of the last nonzero request's seconds; 0 after a stop.
        double sent_seconds = 0.0;
    };

    static void validate_profile(const RumbleProfile& profile);
    static void validate_slot(int input_slot);
    static std::size_t event_index(RumbleEvent event);
    bool session_permits() const noexcept;
    bool slot_permits(std::size_t slot) const noexcept;
    void clear_slot(std::size_t slot, bool force_output) noexcept;
    void publish(std::size_t slot, bool renew) noexcept;

    RumbleSink& sink_;
    const RumbleProfile profile_;
    RumbleSessionState session_;
    std::array<Slot, input_slots::pad_count> slots_{};
};
