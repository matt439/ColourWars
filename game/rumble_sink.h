#pragma once

#include "game/rumble.h"
#include <cstdint>
#include <memory>

// One synchronous device request. The adapter borrows both the nonthrowing
// function and its opaque context, which must outlive the returned sink.
// False means unavailable/disconnected/failed; no exception, retry or claimed
// success follows. SDK types stay inside rumble_xinput.cpp.
using RumbleMotorRequest = bool(void* context, int pad_slot,
    std::uint16_t low_frequency, std::uint16_t high_frequency) noexcept;

// Production factory; construction alone sends no request. Its adapter library
// (colourwars_rumble_xinput) links Windows SDK Xinput.lib, the same XINPUT1_4
// import used by DirectXTK here.
std::unique_ptr<RumbleSink> make_rumble_sink();

// The same adapter/normalization/stop path with a supplied transport. CPU
// checks use this overload and never call XInput. Invalid slots are ignored;
// invalid motor inputs request zero on that slot. Valid inputs round to the
// nearest 16-bit speed. Destruction stops only outstanding nonzero intents;
// an explicit zero clears intent even when the device request fails.
std::unique_ptr<RumbleSink> make_rumble_sink(RumbleMotorRequest& request, void* context);
