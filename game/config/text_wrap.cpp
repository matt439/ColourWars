#include "game/pch.h"
#include "game/config/text_wrap.h"
#include "game/config/content_value.h"
#include "engine/render/render_resources.h"

#include <cmath>
#include <stdexcept>

namespace
{
	float measured_width(const labrador::RenderResources& render,
		labrador::FontHandle font, const std::wstring& text, float scale)
	{
		return render.measure_text(font, text).x * scale;
	}

	// Authored text is printable ASCII by the time it reaches a widget
	// (drawable_text), so a diagnostic can narrow it unit by unit.
	std::string narrow(std::wstring_view text)
	{
		std::string result;
		result.reserve(text.size());
		for (const wchar_t character : text)
		{
			result.push_back(character < 0x80 ? static_cast<char>(character) : '?');
		}
		return result;
	}

	void require_width(float max_width)
	{
		if (!std::isfinite(max_width) || max_width <= 0.0f)
		{
			throw std::invalid_argument(
				"text_wrap::check_fits: max_width must be finite and positive");
		}
	}

}

void text_wrap::check_fits(const labrador::RenderResources& render,
	labrador::FontHandle font, std::wstring_view text, float max_width,
	float scale, const std::string& where)
{
	require_width(max_width);
	size_t index = 0;
	while (index < text.size())
	{
		const size_t word_start = text.find_first_not_of(L" \n", index);
		if (word_start == std::wstring_view::npos)
		{
			return;
		}
		size_t word_end = text.find_first_of(L" \n", word_start);
		if (word_end == std::wstring_view::npos)
		{
			word_end = text.size();
		}
		const std::wstring_view word = text.substr(word_start, word_end - word_start);
		const float width = measured_width(render, font, std::wstring(word), scale);
		if (!(width <= max_width))
		{
			throw std::runtime_error(where + ": the word '" + narrow(word) +
				"' is " + content_value::number_text(width) +
				" wide, wider than the wrap width " +
				content_value::number_text(max_width) + ", so no line break can fit it.");
		}
		index = word_end;
	}
}
