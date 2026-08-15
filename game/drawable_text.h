#pragma once

#include <string>
#include <string_view>

namespace labrador
{
	class RenderResources;
}

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
// A character outside that region used to be an exception. DirectXTK's
// FindGlyph threw "Character not in font" when a font had no default glyph,
// and nothing in either tree called SetDefaultCharacter; both text entry points
// walk glyphs and TextObject measures in its constructor, so the throw landed
// where the text was *built* - for a weapon description, the weapon-select
// page, four screens after the file that carried the character was read,
// arriving as a modal box captioned "startup failure" during play.
//
// THAT IS FIXED IN THE ENGINE NOW - the built-in font kind installs a stand-in
// character at load, so a missing glyph draws a question mark - and this file
// stays anyway, which is a choice and not an oversight. Question marks in a
// weapon description are a worse outcome for THIS game than refusing to start:
// the text is authored content with three readers and one build, so a wrong
// character is always a mistake and never a runtime condition to degrade
// through. The engine is right to degrade; a game whose content is its own is
// right to refuse.
//
// What did change is that the ceiling is no longer only believed. The region
// below is still what the load-time check tests against - it has to be, because
// tuning.json and presentation.json are read before a single font exists - but
// check_font_can_draw asks the actual atlas once the fonts are up, so the
// constant is verified against the thing it describes rather than trusted.
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

	// The same question asked of the font that will actually draw the string,
	// rather than of the constant above.
	//
	// WHY BOTH, AND WHY THIS ONE IS SECOND. widen_drawable runs while
	// presentation.json is being read, which is before the manifest and so
	// before any font exists - it cannot ask an atlas, and moving it later
	// would mean the file that carries the text is no longer the thing being
	// reported on. So the region check stays where the text is read, and this
	// runs once after the fonts are loaded, over the strings that came out of
	// content rather than out of a literal in this repository.
	//
	// It is not redundant with the first. The region constant is a claim about
	// all seven fonts, made by decoding their headers by hand; this is the
	// claim being checked against the font that will draw this string. The two
	// disagree exactly when a font is regenerated with a narrower region than
	// the one the constant records - which no build step prevents, and which
	// would otherwise be found by a player.
	//
	// `where` names the value for the message, as above. Throws naming the
	// character, its index and the font, which is the report the engine's
	// first_unrenderable made possible.
	void check_font_can_draw(const labrador::RenderResources* render_resources,
		const std::string& font_name,
		std::wstring_view text,
		const std::string& where);
}
