#include "game/pch.h"
#include "game/controls_text.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    struct Token
    {
        std::wstring_view token;
        std::wstring label;
        bool required = true;
    };

    // Each required token exactly once and each optional one at most once.
    std::wstring replace_tokens(std::wstring_view authored, const std::vector<Token>& tokens, bool multiline)
    {
        size_t line = 0;
        for (size_t at = 0; at <= authored.size(); ++at)
        {
            if (at < authored.size() && authored[at] != L'\n')
            {
                if (authored[at] < L' ' || authored[at] > L'~')
                { throw std::invalid_argument("Controls text requires printable ASCII and allowed LF breaks."); }
                continue;
            }
            if ((at < authored.size() && !multiline) ||
                authored.substr(line, at - line).find_first_not_of(L' ') == std::wstring_view::npos)
            { throw std::invalid_argument("Controls text requires nonblank lines and its allowed LF breaks."); }
            line = at + 1;
        }
        std::vector<std::pair<size_t, const Token*>> found;
        for (const Token& token : tokens)
        {
            const size_t at = authored.find(token.token);
            if (at != std::wstring_view::npos &&
                authored.find(token.token, at + token.token.size()) != std::wstring_view::npos)
            { throw std::invalid_argument("Controls text permits each key token at most once."); }
            if (at == std::wstring_view::npos)
            {
                if (token.required) { throw std::invalid_argument("Controls text requires its key token exactly once."); }
                continue;
            }
            found.emplace_back(at, &token);
        }
        std::sort(found.begin(), found.end());
        // Inspect original copy, so nested fragments cannot manufacture a token
        // and inserted labels can never be interpreted as another substitution.
        size_t next = 0;
        for (size_t index = 0; index < authored.size(); ++index)
        {
            if (next < found.size() && index == found[next].first)
            {
                index += found[next].second->token.size() - 1; ++next; continue;
            }
            if (authored[index] == L'{' || authored[index] == L'}')
            { throw std::invalid_argument("Controls text permits no other or unmatched braces."); }
        }
        std::wstring result;
        size_t from = 0;
        for (const auto& [at, token] : found)
        {
            result.append(authored.substr(from, at - from));
            result.append(token->label);
            from = at + token->token.size();
        }
        result.append(authored.substr(from));
        return result;
    }

    std::wstring label(const InputBindings& bindings, KeyboardAction action)
    {
        return std::wstring(desktop_control_label(keyboard_control(bindings, action)));
    }

    // Escape always pauses; an extra key follows it.
    std::wstring pause_label(const InputBindings& bindings)
    {
        std::wstring text = L"Esc";
        if (bindings.keyboard_pause != labrador::Key::none)
        {
            text += L" / ";
            text.append(input_key_label(bindings.keyboard_pause));
        }
        return text;
    }

    InputBindings with_jump(labrador::Key jump)
    {
        InputBindings bindings;
        bindings.keyboard_jump = jump;
        return bindings;
    }
}

std::array<labrador::Key, 27> controls_text::jump_keys()
{
	return keyboard_jump_keys();
}

std::wstring controls_text::controls_help(std::wstring_view authored, labrador::Key jump)
{
    // Only the label is checked here; a Jump key that conflicts with a
    // default still formats, as label coverage includes A/D/S.
    InputBindings bindings;
    std::wstring jump_label(input_key_label(jump));
    return replace_tokens(authored, {
        { L"{jump}", jump_label },
        { L"{left}", label(bindings, KeyboardAction::move_left), false },
        { L"{right}", label(bindings, KeyboardAction::move_right), false },
        { L"{drop}", label(bindings, KeyboardAction::drop), false },
        { L"{shoot}", label(bindings, KeyboardAction::shoot), false },
        { L"{pause}", pause_label(bindings), false } }, true);
}

std::wstring controls_text::controls_help(std::wstring_view authored, const InputBindings& bindings)
{
    bindings.validate_keyboard();
    return replace_tokens(authored, {
        { L"{jump}", label(bindings, KeyboardAction::jump) },
        { L"{left}", label(bindings, KeyboardAction::move_left), false },
        { L"{right}", label(bindings, KeyboardAction::move_right), false },
        { L"{drop}", label(bindings, KeyboardAction::drop), false },
        { L"{shoot}", label(bindings, KeyboardAction::shoot), false },
        { L"{pause}", pause_label(bindings), false } }, true);
}

std::wstring controls_text::keyboard_controls_reminder(labrador::Key jump)
{
    // Formatting only: label coverage includes keys that conflict with a default.
    std::wstring text = L"A / D  Move    |    S  Drop through\n";
    text.append(input_key_label(jump));
    text += L"  Jump    |    Left click  Shoot\nMouse  Aim    |    Esc  Pause";
    return text;
}

std::wstring controls_text::keyboard_controls_reminder(const InputBindings& bindings)
{
    bindings.validate_keyboard();
    return label(bindings, KeyboardAction::move_left) + L" / " + label(bindings, KeyboardAction::move_right) +
        L"  Move    |    " + label(bindings, KeyboardAction::drop) + L"  Drop through\n" +
        label(bindings, KeyboardAction::jump) + L"  Jump    |    " + label(bindings, KeyboardAction::shoot) +
        L"  Shoot\nMouse  Aim    |    " + pause_label(bindings) + L"  Pause";
}

std::wstring controls_text::pad_choice_label(const PadActionChoice& choice)
{
    std::wstring text(pad_control_label(choice.primary));
    if (choice.alternate != PadControl::none)
    {
        text += L" / ";
        text.append(pad_control_label(choice.alternate));
    }
    return text;
}

std::wstring controls_text::pad_controls_reminder(const InputBindings& bindings, std::span<const int> slots)
{
    bindings.validate();
    if (slots.empty()) { throw std::invalid_argument("Pad controls reminder needs a physical slot"); }
    const auto profile = [&](int slot) -> const PadProfile&
    {
        return bindings.pads.at(static_cast<size_t>(slot));
    };
    const PadProfile& first = profile(slots.front());
    const bool common = std::all_of(slots.begin(), slots.end(), [&](int slot) { return profile(slot) == first; });
    std::wstring text = L"Left stick  Move    |    Stick down  Drop through\n";
    if (common)
    {
        text += pad_choice_label(first[PadAction::jump]) + L"  Jump    |    " +
            pad_choice_label(first[PadAction::shoot]) + L"  Shoot\n";
        text += L"Right stick  Aim    |    " + pad_choice_label(first[PadAction::pause]) + L"  Pause";
    }
    else
    {
        for (int slot : slots)
        {
            const PadProfile& pad = profile(slot);
            text += L"Pad " + std::to_wstring(slot + 1) + L": " + pad_choice_label(pad[PadAction::jump]) +
                L" Jump, " + pad_choice_label(pad[PadAction::shoot]) + L" Shoot, " +
                pad_choice_label(pad[PadAction::pause]) + L" Pause\n";
        }
        text += L"Right stick  Aim";
    }
    return text;
}

std::wstring controls_text::key_line(std::wstring_view authored, std::wstring_view label)
{
    const auto keys = jump_keys();
    const auto buttons = shoot_mouse_buttons();
    if (std::none_of(keys.begin(), keys.end(), [&](labrador::Key key) { return input_key_label(key) == label; }) &&
        std::none_of(buttons.begin(), buttons.end(), [&](labrador::MouseButton button)
            { return desktop_control_label({ labrador::Key::none, button }) == label; }))
    { throw std::invalid_argument("Controls key label must name A-Z, Space or a Shoot mouse button."); }
    return replace_tokens(authored, { { L"{key}", std::wstring(label) } }, false);
}

std::vector<std::wstring> controls_text::keyboard_control_labels(KeyboardAction action)
{
    std::vector<std::wstring> result;
    if (action == KeyboardAction::pause) { result.emplace_back(desktop_control_label({})); }
    for (const labrador::Key key : jump_keys()) { result.emplace_back(input_key_label(key)); }
    if (action == KeyboardAction::shoot)
    {
        for (const labrador::MouseButton button : shoot_mouse_buttons())
        { result.emplace_back(desktop_control_label({ labrador::Key::none, button })); }
    }
    return result;
}
