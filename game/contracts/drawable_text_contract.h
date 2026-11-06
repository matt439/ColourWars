#pragma once

// Which authored text the game's fonts can draw. The shipped fonts carry the
// ninety-five characters U+0020 to U+007E; the line breaks are layout, not
// glyphs; and the typographic punctuation below is replaced with its ASCII
// spelling before any glyph is looked up. The game's drawable_text reads this
// for every authored string, and the level editor reads it for a stage's
// display name, so the editor refuses exactly the names the game refuses.
// Standard library only, like the rest of game/contracts/.
#include <array>
#include <string_view>

namespace drawable_text_contract
{
    inline constexpr char32_t first_drawable = U' ';
    inline constexpr char32_t last_drawable = U'~';

    // The renderer consumes CR/LF as layout controls; tabs still require glyphs.
    constexpr bool is_layout_control(char32_t character)
    {
        return character == U'\n' || character == U'\r';
    }

    struct Transliteration
    {
        char32_t code_point;
        std::string_view ascii;
    };

    // Typographic punctuation and spaces, and the ASCII each becomes. Authored
    // letters and accents are not transliterated. The code points are escapes
    // so visually similar punctuation and the invisible spaces stay distinct.
    inline constexpr std::array<Transliteration, 16> transliterations = { {
        { U'\u2018', "'" },   // left single quotation mark
        { U'\u2019', "'" },   // right single quotation mark, i.e. the apostrophe
        { U'\u02BC', "'" },   // modifier letter apostrophe
        { U'\u2032', "'" },   // prime
        { U'\u201C', "\"" },  // left double quotation mark
        { U'\u201D', "\"" },  // right double quotation mark
        { U'\u2033', "\"" },  // double prime
        { U'\u2010', "-" },   // hyphen
        { U'\u2011', "-" },   // non-breaking hyphen
        { U'\u2013', "-" },   // en dash
        { U'\u2014', "-" },   // em dash
        { U'\u2212', "-" },   // minus sign
        { U'\u2026', "..." }, // horizontal ellipsis
        { U'\u00A0', " " },   // no-break space
        { U'\u2007', " " },   // figure space
        { U'\u202F', " " } } }; // narrow no-break space

    // The ASCII a character becomes, empty when it is kept as written.
    constexpr std::string_view transliteration(char32_t character)
    {
        for (const Transliteration& entry : transliterations)
        {
            if (entry.code_point == character) { return entry.ascii; }
        }
        return {};
    }
}
