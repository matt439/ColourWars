#pragma once

#include "game/config/play_settings.h"

namespace labrador { class JsonValue; }
class SaveWriter;

// The save's fields for one pad's sticks and comfort switches, shared by the
// play section's per-pad arrays ("pad_sticks", "pad_comfort") and each
// "profiles" entry, which hold them under the same keys.
//
// Each field recovers on its own: a missing or malformed one keeps what
// `destination` already holds. A deadzone is read through
// StickSettings::normalized_deadzone, so one above max_deadzone reads as
// max_deadzone and any negative one as inherit, as the game would use it.
void read_stick_settings(const labrador::JsonValue& entry, StickSettings& destination);
// "movement_deadzone", then "aim_deadzone".
void write_stick_settings(SaveWriter& writer, const StickSettings& sticks);
// An unknown spelling keeps the switch as it was.
void read_seat_comfort(const labrador::JsonValue& entry, SeatComfort& destination);
// "view_shake", "aim_marker", then "reduce_effects".
void write_seat_comfort(SaveWriter& writer, const SeatComfort& comfort);
