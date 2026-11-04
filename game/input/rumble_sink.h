#pragma once

#include "game/input/rumble.h"
#include <memory>

namespace labrador { class Gamepads; }

// Production factory; construction alone sends no request. Its adapter library
// (colourwars_rumble_gamepads) sends through the engine's
// labrador::Gamepads::vibrate and stop_vibration, so no pad API is named here.
// `gamepads` is borrowed: the Application's, which outlives every state.
//
// Invalid slots are ignored. Invalid motor amplitudes, and nonzero motors whose
// seconds are not positive and finite, request a stop on that slot; seconds
// past the engine's max_vibration_seconds are clamped to it. Nothing the
// engine throws or answers leaves set_vibration: unavailable output is not
// retried. Destruction stops only outstanding nonzero intents; an explicit
// zero clears intent even when the device request fails.
//
// Probes that must stay off real motors define this function themselves
// instead of linking the adapter (painter_visual/rumble_sink_fixture.h).
std::unique_ptr<RumbleSink> make_rumble_sink(labrador::Gamepads& gamepads);
