#pragma once

#include "game/input_bindings.h"
#include <array>
#include <string>
#include <string_view>

namespace controls_text
{
    // Label coverage includes A/D/S even though those keys conflict with other
    // gameplay actions. Formatting is not input-preference validation.
    std::array<labrador::Key, 27> jump_keys();
    std::wstring controls_help(std::wstring_view authored, labrador::Key jump);
    std::wstring keyboard_controls_reminder(labrador::Key jump);
    std::wstring key_line(std::wstring_view authored, std::wstring_view label);
}
