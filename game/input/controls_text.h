#pragma once

#include "game/input/controls_copy.h"
#include "game/input/input_bindings.h"
#include <array>
#include <string>
#include <string_view>
#include <span>
#include <vector>

// The player-facing controls text, formatted with menu_text.controls
// (controls_copy.h), so a locale reaches every word of it. The label
// functions take the words from the copy and the legends (A-Z, the pad
// buttons) from input_bindings.h, which holds no words.
namespace controls_text
{
    // Label coverage includes A/D/S even though those keys conflict with other
    // gameplay actions. Formatting is not input-preference validation.
    std::array<labrador::Key, 27> jump_keys();
    // "A" ... "Z" as legends, then the copy's Space.
    std::wstring key_label(const ControlLabels& labels, labrador::Key key);
    // A key, a Shoot mouse button, or the copy's None for the unbound extra Pause key.
    std::wstring desktop_label(const ControlLabels& labels, const DesktopControl& control);
    // The pad legends, then the copy's Start and None.
    std::wstring pad_label(const ControlLabels& labels, PadControl control);
    // {jump} is required exactly once; {left}, {right}, {drop}, {shoot} and
    // {pause} (Esc, then any extra key) are optional, each at most once, and
    // so are {pad_jump}, {pad_shoot} and {pad_pause}: the pads' shared choice
    // ("A / LT"), or the copy's per_pad when the slots' profiles differ. The
    // Key overload formats the defaults with that Jump label.
    std::wstring controls_help(const ControlsCopy& copy, std::wstring_view authored, labrador::Key jump);
    std::wstring controls_help(const ControlsCopy& copy, std::wstring_view authored, const InputBindings& bindings);
    std::wstring keyboard_controls_reminder(const ControlsCopy& copy, labrador::Key jump);
    std::wstring keyboard_controls_reminder(const ControlsCopy& copy, const InputBindings& bindings);
    // Every label an action's row can show, for fit validation: None for
    // Pause, the 27 keys, and the mouse buttons for Shoot.
    std::vector<std::wstring> keyboard_control_labels(const ControlLabels& labels, KeyboardAction action);
    // "A / LT": an action's primary control, then its alternate if any.
    std::wstring pad_choice_label(const ControlLabels& labels, const PadActionChoice& choice);
    // Shared reminder for the physical pad slots actually present. A common
    // profile uses gamepad_reminder; mixed profiles name each slot.
    std::wstring pad_controls_reminder(const ControlsCopy& copy, const InputBindings& bindings,
        std::span<const int> slots);
    // {key} exactly once, single-line; `label` must be one of key_label's or
    // a Shoot mouse button's in `labels`.
    std::wstring key_line(const ControlLabels& labels, std::wstring_view authored, std::wstring_view label);
}
