#pragma once

#include <string>
#include <string_view>

// The one place that knows what this game's fonts can actually draw.
//
// Every .spritefont in this tree - all thirty-three of them, the engine
// sample's included - carries exactly ninety-five glyphs: U+0020 to U+007E,
// contiguous, with no default character set. That is MakeSpriteFont's default
// region, which nothing chose and nothing wrote down, and the fonts cannot be
// regenerated: the cmd.txt beside them describes none of the seven the game
// loads and passes no /CharacterRegion, so the ceiling is effectively
// permanent until a font recipe exists.
//
// A character outside that region is not a blank box. DirectXTK's FindGlyph
// falls back to a default glyph and throws "Character not in font" when there
// is none, and nothing in the engine, the samples or this game ever calls
// SetDefaultCharacter. Both text entry points walk glyphs and TextObject
// measures in its constructor, so the throw lands where the text is *built* -
// which for a weapon description is the weapon-select page, four screens after
// the file that carried the character was read, arriving as a modal box
// captioned "startup failure" during play.
//
// So content text is checked where it is read, like every other content value
// in this game, rather than being trusted to a font that will throw for it.
namespace drawable_text
{
	// The region every font in the tree covers.
	constexpr wchar_t first_drawable = L' ';
	constexpr wchar_t last_drawable = L'~';

	// And the two that are drawable without being glyphs at all. DirectXTK's
	// glyph walk answers both of these itself - a carriage return is skipped
	// and a line feed moves down a line - before it ever looks a character up,
	// so neither reaches the throw. The shipped weapon descriptions rely on
	// it: their second sentence is on a second line, put there by a literal
	// \n typed into presentation.json.
	//
	// Nothing else is exempt. A tab is not special-cased by that walk, so it
	// goes to the glyph table like any other character and is not in it.
	constexpr bool is_layout_control(wchar_t character)
	{
		return character == L'\n' || character == L'\r';
	}

	// Converts UTF-8 content text to the wide string the render module wants,
	// transliterating the punctuation an editor inserts on its own, and
	// throwing if anything is left that no font here can draw.
	//
	// `where` names the value for the message, and wants to be whatever
	// JsonValue::where() would have said: the file and the key.
	//
	// Callers that keep their text narrow and widen it somewhere else should
	// still call this at load and discard the result - it is the check that
	// matters, and calling the same function in both places is what keeps the
	// two from disagreeing about which characters are acceptable.
	std::wstring widen_drawable(std::string_view utf8, const std::string& where);
}
