#include "game/pch.h"
#include "game/config/drawable_text.h"
#include "engine/render/render_resources.h"
#include "engine/render/text_encoding.h"

namespace
{
	// Normalize typographic punctuation, but preserve authored letters and
	// accents. The table is drawable_text_contract's, which the editor reads too.
	std::string_view transliterate(wchar_t character)
	{
		return drawable_text_contract::transliteration(static_cast<char32_t>(character));
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
			const std::string_view replacement = transliterate(text[index]);
			if (replacement.empty())
			{
				++index;
				continue;
			}

			text.replace(index, 1, std::wstring(replacement.begin(), replacement.end()));
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
