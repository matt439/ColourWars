#pragma once

#include "game/contracts/drawable_text_contract.h"
#include <string>
#include <string_view>

namespace labrador
{
	class RenderResources;
}

// Authored text is restricted to printable ASCII plus line breaks, after
// punctuation normalization. Reject unsupported content even though the engine
// can substitute missing glyphs. Once fonts load, check each authored string
// against the font that will draw it.
namespace drawable_text
{
	// Character range accepted by the content parser, from the contract the
	// level editor also reads.
	constexpr wchar_t first_drawable = static_cast<wchar_t>(drawable_text_contract::first_drawable);
	constexpr wchar_t last_drawable = static_cast<wchar_t>(drawable_text_contract::last_drawable);

	// The renderer consumes CR/LF as layout controls; tabs still require glyphs.
	constexpr bool is_layout_control(wchar_t character)
	{
		return drawable_text_contract::is_layout_control(static_cast<char32_t>(character));
	}

	// Normalize UTF-8 punctuation and reject characters outside the content range.
	// Use a file/key description for where. Call during loading even when a caller
	// stores the original narrow text and converts it again for display.
	std::wstring widen_drawable(std::string_view utf8, const std::string& where);

	// Call after fonts load to verify this string against the actual atlas.
	// Throws with where, the missing character's index, and the font name.
	void check_font_can_draw(const labrador::RenderResources* render_resources,
		const std::string& font_name,
		std::wstring_view text,
		const std::string& where);
}
