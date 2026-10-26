#pragma once

#include "game/rumble.h"

// Feedback only: coalesce repeated occurrences until the match owner drains
// them after a completed tick. No device, clock, random state or gameplay
// decision belongs here. Blocked consumers must still drain/discard events.
struct RumbleEvents
{
    bool hit = false;
    bool elimination = false;
    bool hard_landing = false;
    bool dry_fire = false;

    bool empty() const noexcept
    {
        return !hit && !elimination && !hard_landing && !dry_fire;
    }
    bool contains(RumbleEvent event) const noexcept
    {
        switch (event)
        {
        case RumbleEvent::hit: return hit;
        case RumbleEvent::elimination: return elimination;
        case RumbleEvent::hard_landing: return hard_landing;
        case RumbleEvent::dry_fire: return dry_fire;
        default: return false;
        }
    }
    void merge(const RumbleEvents& other) noexcept
    {
        hit = hit || other.hit;
        elimination = elimination || other.elimination;
        hard_landing = hard_landing || other.hard_landing;
        dry_fire = dry_fire || other.dry_fire;
    }
    bool operator==(const RumbleEvents&) const = default;
};

struct SlotRumbleEvents
{
    // The Player's physical input slot, independent of view/roster order.
    // Keyboard and unclaimed slots are retained for the consumer to discard.
    int input_slot = -1;
    RumbleEvents events;
};
