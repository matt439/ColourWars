#include "game/pch.h"
#include "game/text_wrap.h"
#include "game/content_value.h"
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

	void require_width(float max_width, const char* caller)
	{
		if (!std::isfinite(max_width) || max_width <= 0.0f)
		{
			throw std::invalid_argument(std::string("text_wrap::") + caller +
				": max_width must be finite and positive");
		}
	}

	void wrap_line(const labrador::RenderResources& render,
		labrador::FontHandle font, std::wstring_view line, float max_width,
		float scale, std::wstring& output)
	{
		std::wstring current;
		size_t index = 0;
		while (index < line.size())
		{
			const size_t word_start = line.find_first_not_of(L' ', index);
			if (word_start == std::wstring_view::npos)
			{
				// Trailing spaces draw nothing; keep them as authored.
				current.append(line.substr(index));
				break;
			}
			size_t word_end = line.find(L' ', word_start);
			if (word_end == std::wstring_view::npos)
			{
				word_end = line.size();
			}
			std::wstring candidate = current;
			candidate.append(line.substr(index, word_end - index));
			if (!current.empty() &&
				measured_width(render, font, candidate, scale) > max_width)
			{
				output.append(current);
				output.push_back(L'\n');
				current.assign(line.substr(word_start, word_end - word_start));
			}
			else
			{
				current = std::move(candidate);
			}
			index = word_end;
		}
		output.append(current);
	}
}

std::wstring text_wrap::wrap(const labrador::RenderResources& render,
	labrador::FontHandle font, std::wstring_view text, float max_width,
	float scale)
{
	require_width(max_width, "wrap");
	std::wstring output;
	output.reserve(text.size());
	size_t line_start = 0;
	while (true)
	{
		const size_t line_end = text.find(L'\n', line_start);
		wrap_line(render, font, text.substr(line_start,
			line_end == std::wstring_view::npos ? std::wstring_view::npos
				: line_end - line_start), max_width, scale, output);
		if (line_end == std::wstring_view::npos)
		{
			return output;
		}
		output.push_back(L'\n');
		line_start = line_end + 1;
	}
}

void text_wrap::check_fits(const labrador::RenderResources& render,
	labrador::FontHandle font, std::wstring_view text, float max_width,
	float scale, const std::string& where)
{
	require_width(max_width, "check_fits");
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
