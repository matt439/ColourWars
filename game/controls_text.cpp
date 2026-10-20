#include "game/pch.h"
#include "game/controls_text.h"
#include <algorithm>
#include <stdexcept>

namespace
{
    std::wstring replace_key(std::wstring_view authored, std::wstring_view token,
        std::wstring_view label, bool multiline)
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
        const size_t at = authored.find(token);
        if (at == std::wstring_view::npos || authored.find(token, at + token.size()) != std::wstring_view::npos)
        { throw std::invalid_argument("Controls text requires its key token exactly once."); }
        // Inspect original copy, so nested fragments cannot manufacture a token
        // and inserted labels can never be interpreted as another substitution.
        for (size_t index = 0; index < authored.size(); ++index)
        {
            if (index == at) { index += token.size() - 1; continue; }
            if (authored[index] == L'{' || authored[index] == L'}')
            { throw std::invalid_argument("Controls text permits no other or unmatched braces."); }
        }
        std::wstring result(authored.substr(0, at));
        result.append(label); result.append(authored.substr(at + token.size()));
        return result;
    }
}

std::array<labrador::Key, 27> controls_text::jump_keys()
{
	return keyboard_jump_keys();
}

std::wstring controls_text::controls_help(std::wstring_view authored, labrador::Key jump)
{
    return replace_key(authored, L"{jump}", input_key_label(jump), true);
}

std::wstring controls_text::keyboard_controls_reminder(labrador::Key jump)
{
    std::wstring text = L"A / D  Move    |    S  Drop through\n";
    text.append(input_key_label(jump));
    text += L"  Jump    |    Left click  Shoot\nMouse  Aim    |    Esc  Pause";
    return text;
}

std::wstring controls_text::key_line(std::wstring_view authored, std::wstring_view label)
{
    const auto keys = jump_keys();
    if (std::none_of(keys.begin(), keys.end(), [&](labrador::Key key) { return input_key_label(key) == label; }))
    { throw std::invalid_argument("Controls key label must name A-Z or Space."); }
    return replace_key(authored, L"{key}", label, false);
}
