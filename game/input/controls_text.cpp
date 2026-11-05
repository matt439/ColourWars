#include "game/pch.h"
#include "game/input/controls_text.h"
#include "game/config/drawable_text.h"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <tuple>
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
                if (authored[at] < drawable_text::first_drawable || authored[at] > drawable_text::last_drawable)
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

    std::wstring label(const ControlLabels& labels, const InputBindings& bindings, KeyboardAction action)
    {
        return controls_text::desktop_label(labels, keyboard_control(bindings, action));
    }

    // Escape always pauses; an extra key follows it.
    std::wstring pause_label(const ControlLabels& labels, const InputBindings& bindings)
    {
        std::wstring text = labels.escape;
        if (bindings.keyboard_pause != labrador::Key::none)
        {
            text += L" / ";
            text += controls_text::key_label(labels, bindings.keyboard_pause);
        }
        return text;
    }

    // {jump} is always required; the other five are required by the reminder
    // and optional in the Controls help.
    std::vector<Token> keyboard_tokens(const ControlLabels& labels, const InputBindings& bindings,
        bool all_required)
    {
        return {
            { L"{jump}", label(labels, bindings, KeyboardAction::jump) },
            { L"{left}", label(labels, bindings, KeyboardAction::move_left), all_required },
            { L"{right}", label(labels, bindings, KeyboardAction::move_right), all_required },
            { L"{drop}", label(labels, bindings, KeyboardAction::drop), all_required },
            { L"{shoot}", label(labels, bindings, KeyboardAction::shoot), all_required },
            { L"{pause}", pause_label(labels, bindings), all_required } };
    }

    std::vector<Token> choice_tokens(const ControlLabels& labels, const PadProfile& pad)
    {
        return {
            { L"{pad_jump}", controls_text::pad_choice_label(labels, pad[PadAction::jump]) },
            { L"{pad_shoot}", controls_text::pad_choice_label(labels, pad[PadAction::shoot]) },
            { L"{pad_pause}", controls_text::pad_choice_label(labels, pad[PadAction::pause]) } };
    }

    // The help's pad lines name the shared profile's choices. Profiles that
    // differ by slot are named per pad on the countdown and on Pad buttons.
    std::vector<Token> with_pad_tokens(std::vector<Token> tokens, const ControlsCopy& copy,
        const InputBindings& bindings)
    {
        const PadProfile& first = bindings.pads.front();
        const bool common = std::all_of(bindings.pads.begin(), bindings.pads.end(),
            [&](const PadProfile& pad) { return pad == first; });
        for (Token& token : choice_tokens(copy.labels, first))
        {
            if (!common) { token.label = copy.per_pad; }
            token.required = false;
            tokens.push_back(std::move(token));
        }
        return tokens;
    }

    // The defaults with that Jump key. Its label is checked here, as none
    // would otherwise read as the unbound control.
    InputBindings with_jump(labrador::Key jump)
    {
        std::ignore = input_key_legend(jump);
        InputBindings bindings;
        bindings.keyboard_jump = jump;
        return bindings;
    }

    const std::wstring& mouse_label(const ControlLabels& labels, labrador::MouseButton button)
    {
        switch (button)
        {
        case labrador::MouseButton::left: return labels.mouse_left;
        case labrador::MouseButton::right: return labels.mouse_right;
        case labrador::MouseButton::middle: return labels.mouse_middle;
        case labrador::MouseButton::x1: return labels.mouse_x1;
        case labrador::MouseButton::x2: return labels.mouse_x2;
        default: throw std::invalid_argument("Unsupported mouse button");
        }
    }
}

std::array<labrador::Key, 27> controls_text::jump_keys()
{
    return keyboard_jump_keys();
}

std::wstring controls_text::key_label(const ControlLabels& labels, labrador::Key key)
{
    // input_key_legend refuses a key outside A-Z and Space.
    const std::wstring_view legend = input_key_legend(key);
    return key == labrador::Key::space ? labels.space : std::wstring(legend);
}

std::wstring controls_text::desktop_label(const ControlLabels& labels, const DesktopControl& control)
{
    // desktop_control_name refuses a control that is both, an unsupported button
    // or a key outside A-Z and Space.
    std::ignore = desktop_control_name(control);
    if (control.key == labrador::Key::none && control.button == labrador::MouseButton::none) { return labels.none; }
    if (control.button != labrador::MouseButton::none) { return mouse_label(labels, control.button); }
    return key_label(labels, control.key);
}

std::wstring controls_text::pad_label(const ControlLabels& labels, PadControl control)
{
    // pad_control_legend refuses an unknown control.
    const std::wstring_view legend = pad_control_legend(control);
    if (control == PadControl::none) { return labels.none; }
    if (control == PadControl::start) { return labels.start; }
    return std::wstring(legend);
}

std::wstring controls_text::controls_help(const ControlsCopy& copy, std::wstring_view authored, labrador::Key jump)
{
    // Only the label is checked here; a Jump key that conflicts with a
    // default still formats, as label coverage includes A/D/S.
    const InputBindings bindings = with_jump(jump);
    return replace_tokens(authored,
        with_pad_tokens(keyboard_tokens(copy.labels, bindings, false), copy, bindings), true);
}

std::wstring controls_text::controls_help(const ControlsCopy& copy, std::wstring_view authored,
    const InputBindings& bindings)
{
    bindings.validate_keyboard();
    return replace_tokens(authored,
        with_pad_tokens(keyboard_tokens(copy.labels, bindings, false), copy, bindings), true);
}

std::wstring controls_text::keyboard_controls_reminder(const ControlsCopy& copy, labrador::Key jump)
{
    // Formatting only: label coverage includes keys that conflict with a default.
    return replace_tokens(copy.keyboard_reminder, keyboard_tokens(copy.labels, with_jump(jump), true), true);
}

std::wstring controls_text::keyboard_controls_reminder(const ControlsCopy& copy, const InputBindings& bindings)
{
    bindings.validate_keyboard();
    return replace_tokens(copy.keyboard_reminder, keyboard_tokens(copy.labels, bindings, true), true);
}

std::wstring controls_text::pad_choice_label(const ControlLabels& labels, const PadActionChoice& choice)
{
    std::wstring text = pad_label(labels, choice.primary);
    if (choice.alternate != PadControl::none)
    {
        text += L" / ";
        text += pad_label(labels, choice.alternate);
    }
    return text;
}

std::wstring controls_text::pad_controls_reminder(const ControlsCopy& copy, const InputBindings& bindings,
    std::span<const int> slots)
{
    bindings.validate();
    if (slots.empty()) { throw std::invalid_argument("Pad controls reminder needs a physical slot"); }
    const auto profile = [&](int slot) -> const PadProfile&
    {
        return bindings.pads.at(static_cast<size_t>(slot));
    };
    const PadProfile& first = profile(slots.front());
    const bool common = std::all_of(slots.begin(), slots.end(), [&](int slot) { return profile(slot) == first; });
    if (common) { return replace_tokens(copy.gamepad_reminder, choice_tokens(copy.labels, first), true); }
    // One line per present slot, LF between them; replace_tokens inserts the
    // joined lines verbatim, so the breaks they carry are not re-read.
    std::wstring pads;
    for (int slot : slots)
    {
        std::vector<Token> tokens = choice_tokens(copy.labels, profile(slot));
        tokens.push_back({ L"{pad}", std::to_wstring(slot + 1) });
        if (!pads.empty()) { pads += L'\n'; }
        pads += replace_tokens(copy.gamepad_pad_line, tokens, false);
    }
    return replace_tokens(copy.gamepad_per_pad_reminder, { { L"{pads}", pads } }, true);
}

std::wstring controls_text::key_line(const ControlLabels& labels, std::wstring_view authored, std::wstring_view label)
{
    const auto keys = jump_keys();
    const auto buttons = shoot_mouse_buttons();
    if (std::none_of(keys.begin(), keys.end(), [&](labrador::Key key) { return key_label(labels, key) == label; }) &&
        std::none_of(buttons.begin(), buttons.end(), [&](labrador::MouseButton button)
            { return mouse_label(labels, button) == label; }))
    { throw std::invalid_argument("Controls key label must name A-Z, Space or a Shoot mouse button."); }
    return replace_tokens(authored, { { L"{key}", std::wstring(label) } }, false);
}

std::vector<std::wstring> controls_text::keyboard_control_labels(const ControlLabels& labels, KeyboardAction action)
{
    std::vector<std::wstring> result;
    if (action == KeyboardAction::pause) { result.push_back(labels.none); }
    for (const labrador::Key key : jump_keys()) { result.push_back(key_label(labels, key)); }
    if (action == KeyboardAction::shoot)
    {
        for (const labrador::MouseButton button : shoot_mouse_buttons())
        { result.push_back(mouse_label(labels, button)); }
    }
    return result;
}
