// Synthetic client workaround, not engine implementation or game source.
// This representative subset deliberately does not name every engine Key.
#include "engine/input/keyboard.h"

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string_view>

namespace client_workaround
{
    using labrador::Key;

    struct KeyToken
    {
        Key key;
        std::string_view token;
    };

    // These stable ASCII tokens identify engine key positions, not the
    // character typed or a localized label. Explicit entries avoid ordinals.
    constexpr std::array<KeyToken, 9> key_tokens{ {
        { Key::a, "a" }, { Key::j, "j" }, { Key::digit_1, "1" },
        { Key::f5, "f5" }, { Key::space, "space" }, { Key::del, "delete" },
        { Key::page_up, "pageup" }, { Key::left_bracket, "lbracket" },
        { Key::numpad_1, "numpad1" }
    } };

    constexpr std::string_view key_name(Key key)
    {
        for (const auto& entry : key_tokens)
        {
            if (entry.key == key) { return entry.token; }
        }
        return {};
    }

    constexpr std::optional<Key> key_from_name(std::string_view token)
    {
        for (const auto& entry : key_tokens)
        {
            if (entry.token == token) { return entry.key; }
        }
        return std::nullopt;
    }

    constexpr bool table_is_unambiguous()
    {
        for (std::size_t index = 0; index < key_tokens.size(); ++index)
        {
            const auto& entry = key_tokens[index];
            if (entry.token.empty() || entry.key == Key::none || entry.key == Key::count)
                { return false; }
            for (std::size_t other = index + 1; other < key_tokens.size(); ++other)
            {
                if (entry.key == key_tokens[other].key || entry.token == key_tokens[other].token)
                    { return false; }
            }
        }
        return true;
    }

    static_assert(table_is_unambiguous());
    static_assert(key_name(Key::j) == "j");
    static_assert(key_from_name("space") == Key::space);
    static_assert(key_name(Key::digit_1) != key_name(Key::numpad_1));
    static_assert(key_name(Key::none).empty() && key_name(Key::count).empty());
    static_assert(!key_from_name("unknown").has_value());

    void require(bool condition, const char* reason)
    {
        if (!condition)
        {
            std::fprintf(stderr, "FAIL canonical key names: %s\n", reason);
            std::exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    using labrador::Key;
    using client_workaround::key_from_name;
    using client_workaround::key_name;
    using client_workaround::require;

    // These checks stay enabled in Release; they do not use assert/NDEBUG.
    for (const auto& entry : client_workaround::key_tokens)
    {
        require(key_name(entry.key) == entry.token, "format changed a canonical token");
        const auto parsed = key_from_name(entry.token);
        require(parsed.has_value() && *parsed == entry.key, "parse(format(key)) failed");
        require(key_name(*parsed) == entry.token, "format(parse(token)) failed");
    }
    for (const std::string_view invalid : { "", "J", "Space", "j ", " j",
        "ctrl+j", "shift+f5", "KeyJ", "numpad_1", "unknown" })
    {
        require(!key_from_name(invalid).has_value(), "accepted a noncanonical token or chord");
    }
    require(!key_from_name(std::string_view("j\0", 2)).has_value(), "accepted an embedded NUL");
    require(key_name(Key::none).empty() && key_name(Key::count).empty(), "named an enum sentinel");
    require(key_name(static_cast<Key>(255)).empty(), "named an invalid enum value");
    require(key_name(Key::z).empty(), "partial exhibit unexpectedly claims complete enum coverage");
    require(key_from_name("1") == Key::digit_1 && key_from_name("numpad1") == Key::numpad_1,
        "number row and numpad lost their distinct key identities");
    std::puts("PASS canonical key names: nine explicit physical-key tokens; two-way round trips, distinct number/numpad, strict errors and sentinels; no window, devices or content");
    return EXIT_SUCCESS;
}
