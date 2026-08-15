#include "game/pch.h"
#include "game/drawable_text.h"
#include "engine/render/render_resources.h"
#include "engine/render/text_encoding.h"

namespace
{
	// The characters a text editor produces without being asked for them: a
	// smart quote for a straight one, a dash for two hyphens, a non-breaking
	// space for a space. Word does it, Visual Studio does it, and pasting from
	// anywhere at all does it.
	//
	// The test for being on this list is that the substitution does not change
	// what the text says. An accented letter is not on it: folding an accent
	// away changes a word rather than its typography, and what a stage called
	// Cafe should be named is a decision for whoever names it, not for this.
	//
	// Spelt as escapes rather than as the characters themselves, deliberately.
	// This would otherwise be the one source file in the repository that is
	// not plain ASCII, its meaning would depend on the compiler agreeing with
	// the editor about its encoding - which is the whole subject here - and
	// three of the entries are invisible spaces no reader could tell apart.
	std::wstring_view transliterate(wchar_t character)
	{
		switch (character)
		{
		case L'\u2018':	// left single quotation mark
		case L'\u2019':	// right single quotation mark, i.e. the apostrophe
		case L'\u02BC':	// modifier letter apostrophe
		case L'\u2032':	// prime
			return L"'";

		case L'\u201C':	// left double quotation mark
		case L'\u201D':	// right double quotation mark
		case L'\u2033':	// double prime
			return L"\"";

		case L'\u2010':	// hyphen
		case L'\u2011':	// non-breaking hyphen
		case L'\u2013':	// en dash
		case L'\u2014':	// em dash
		case L'\u2212':	// minus sign
			return L"-";

		case L'\u2026':	// horizontal ellipsis
			return L"...";

		case L'\u00A0':	// no-break space
		case L'\u2007':	// figure space
		case L'\u202F':	// narrow no-break space
			return L" ";

		default:
			return {};
		}
	}

	std::string code_point_text(wchar_t character)
	{
		constexpr char digits[] = "0123456789ABCDEF";

		std::string text = "U+";
		for (int shift = 12; shift >= 0; shift -= 4)
		{
			text += digits[(static_cast<unsigned>(character) >> shift) & 0xF];
		}
		return text;
	}
}

namespace drawable_text
{
	std::wstring widen_drawable(std::string_view utf8, const std::string& where)
	{
		std::wstring text = labrador::widen(utf8);

		for (size_t index = 0; index < text.size(); )
		{
			const std::wstring_view replacement = transliterate(text[index]);
			if (replacement.empty())
			{
				++index;
				continue;
			}

			text.replace(index, 1, replacement);
			index += replacement.size();
		}

		for (const wchar_t character : text)
		{
			if ((character >= first_drawable && character <= last_drawable) ||
				is_layout_control(character))
			{
				continue;
			}

			// Named where it can still be fixed, rather than thrown by
			// DirectXTK from inside a widget constructor several screens later
			// with nothing in the message about which file it came from.
			//
			// U+FFFD reaches here by a route worth knowing about: widen()
			// promises invalid UTF-8 becomes a replacement character so that
			// "text should show mojibake, not vanish", and no font in the tree
			// can draw one - so a content file saved in the wrong encoding
			// takes the engine's documented graceful path straight into the
			// throw it was written to avoid. It gets the same sentence as
			// anything else.
			throw std::runtime_error(where + " contains " +
				code_point_text(character) + ", which none of this game's "
				"fonts has a glyph for. They carry the ninety-five characters "
				"U+0020 to U+007E and nothing else, and a character outside "
				"that range is not a blank space - it is an exception thrown "
				"when the text is built.");
		}

		return text;
	}

	void check_font_can_draw(const labrador::RenderResources* render_resources,
		const std::string& font_name,
		std::wstring_view text,
		const std::string& where)
	{
		const labrador::FontHandle font =
			render_resources->resolve_sprite_font(font_name);

		// One call over the whole string. This used to walk the runs between
		// layout controls, because the query answered "no glyph" about the
		// newline in every weapon description - it asked the atlas about
		// characters the pen walk consumes without ever looking them up. That
		// was report 8, and the engine's f94ac81 made the query skip what the
		// walk skips, off one definition of the set. So the filter here is
		// gone rather than kept in step with a private one.
		const size_t index = render_resources->first_unrenderable(font, text);
		if (index == std::wstring_view::npos)
		{
			return;
		}

		// The index as well as the character, which is the half of this the
		// load-time check cannot give: it walks a string it has just built and
		// reports the first character it dislikes, while this one is reading
		// an answer out of the atlas and is told where.
		throw std::runtime_error(where + " contains " +
			code_point_text(text[index]) + " at index " +
			std::to_string(index) + ", which the font '" + font_name +
			"' has no glyph for. The load-time check passed it, so that font "
			"covers less than the ninety-five characters U+0020 to U+007E "
			"this game's content is written against - which means the font "
			"was regenerated with a narrower region, not that the text is "
			"wrong.");
	}
}
