#pragma once

// For probes that script a Level by device slot, as the game's readers hand it
// input: the slot-indexed vector mapped to one command per participant, which
// is what Level::update takes. The mapping is SeatCommandBuilder's own, run
// without a mouse, so a keyboard seat's aim is whatever the probe wrote; a
// probe that tests mouse aim builds a SeatCommandBuilder with its mouse.

#include "game/objects/level.h"
#include "game/objects/seat_commands.h"
#include <vector>

namespace acceptance
{
    // Slot commands with no pause or intro-skip signal yet.
    inline std::vector<SlotInput> slot_inputs(const std::vector<PlayerInputData>& slots)
    {
        std::vector<SlotInput> devices(slots.size());
        for (std::size_t slot = 0; slot < slots.size(); ++slot)
        {
            static_cast<PlayerInputData&>(devices[slot]) = slots[slot];
        }
        return devices;
    }

    inline std::vector<PlayerInputData> slot_seats(const Level& level,
        const std::vector<PlayerInputData>& slots)
    {
        return SeatCommandBuilder(nullptr, nullptr).build(level, slot_inputs(slots)).seats;
    }

    // One step from slot input, as GameLevel takes it: a participant's intro
    // skip becomes the level's request.
    inline void update_from_slots(Level& level, const std::vector<SlotInput>& slots,
        float dt, SeatInputFilter* seats = nullptr)
    {
        const SeatCommands commands = SeatCommandBuilder(nullptr, nullptr).build(level, slots);
        if (commands.skip_intro) { level.request_intro_skip(); }
        level.update(commands.seats, dt, seats);
    }
}
