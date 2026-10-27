#pragma once

#include "game/input_bindings.h"
#include <array>
#include <string>
#include <string_view>
#include <span>

namespace controls_text
{
    // Label coverage includes A/D/S even though those keys conflict with other
    // gameplay actions. Formatting is not input-preference validation.
    std::array<labrador::Key, 27> jump_keys();
    std::wstring controls_help(std::wstring_view authored, labrador::Key jump);
    std::wstring keyboard_controls_reminder(labrador::Key jump);
    // "A / LT": an action's primary control, then its alternate if any.
    std::wstring pad_choice_label(const PadActionChoice& choice);
    // Shared reminder for the physical pad slots actually present. A common
    // profile uses the two-column layout; mixed profiles name each slot.
    std::wstring pad_controls_reminder(const InputBindings& bindings, std::span<const int> slots);
    std::wstring key_line(std::wstring_view authored, std::wstring_view label);
}
