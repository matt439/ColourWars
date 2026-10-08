#include "game/pch.h"
#include "game/drawable_text.h"
#include "engine/render/render_resources.h"
#include "engine/render/text_encoding.h"

namespace
{
	// Normalize typographic punctuation, but preserve authored letters and accents.
	// Escapes distinguish visually similar punctuation and invisible spaces.
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

			// Report the authored location. Invalid UTF-8 also fails here because
			// widen() replaces it with U+FFFD, outside the accepted content range.
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

		// The engine's query handles layout controls consistently with text drawing.
		const size_t index = render_resources->first_unrenderable(font, text);
		if (index == std::wstring_view::npos)
		{
			return;
		}

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
