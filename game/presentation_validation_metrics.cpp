#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/player_profiles.h"
#include "game/objects/player_badge.h"
#include "game/resource_validation.h"
#include "game/controls_text.h"
#include "game/states/ui_player_badge.h"
#include "game/text_wrap.h"
#include "engine/render/sprite_geometry.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;
using namespace presentation_validation;

namespace presentation_validation
{

	void append_label_glyph(LabelPen& pen, const Glyph& glyph, wchar_t character,
		float line_spacing, const std::string& where)
	{
		if (!std::isfinite(glyph.x_offset) || !std::isfinite(glyph.y_offset) ||
			!std::isfinite(glyph.x_advance))
		{
			throw std::runtime_error(where + " requires finite font glyph metrics.");
		}
		// Keep Font::for_each_glyph/measure's FLOAT operation order, including
		// carried bearings, the zero clamp, whitespace suppression and overhang.
		pen.x += glyph.x_offset;
		if (pen.x < 0.0f) { pen.x = 0.0f; }
		const float width = static_cast<float>(glyph.subrect.width);
		if (character != L' ' || glyph.subrect.width > 1 || glyph.subrect.height > 1)
		{
			const float height = character == L' ' ? line_spacing :
				std::max(line_spacing, static_cast<float>(glyph.subrect.height) + glyph.y_offset);
			pen.extent.x = std::max(pen.extent.x, pen.x + width);
			pen.extent.y = std::max(pen.extent.y, pen.y + height);
		}
		pen.x += width + glyph.x_advance;
		if (!std::isfinite(pen.x) || !std::isfinite(pen.y) ||
			!std::isfinite(pen.extent.x) || !std::isfinite(pen.extent.y))
		{
			throw std::runtime_error(where + " requires finite composed text metrics.");
		}
	}

	void append_label_text(LabelPen& pen, const Font& font, std::wstring_view text,
		const std::string& where)
	{
		if (!std::isfinite(font.line_spacing()) || font.line_spacing() <= 0.0f)
		{
			throw std::runtime_error(where + " requires finite positive font line spacing.");
		}
		for (const wchar_t character : text)
		{
			if (character == L'\r') { continue; }
			if (character == L'\n')
			{
				pen.x = 0.0f;
				pen.y += font.line_spacing();
				continue;
			}
			append_label_glyph(pen, font.drawn(static_cast<char32_t>(character)),
				character, font.line_spacing(), where);
		}
	}

	Vector2F round_heading_size(const Font& font, std::wstring_view text, const std::string& where)
	{
		if (!std::isfinite(font.line_spacing()))
		{
			throw std::runtime_error(where + " requires finite font line spacing.");
		}
		// The caller has checked the original, single-line template and every
		// glyph. Reuse the existing finite-metric glyph walk, but do not impose
		// append_label_text's positive line-spacing rule on this heading role.
		struct State
		{
			bool present = false;
			LabelPen maximum;
			float minimum_height = 0.0f;
			bool zero_width = false;
			float zero_width_pen = 0.0f;
		};
		const auto append = [&](State& state, wchar_t character)
		{
			const Glyph& glyph = font.drawn(static_cast<char32_t>(character));
			LabelPen maximum = state.maximum;
			LabelPen minimum_height = state.maximum;
			minimum_height.extent.y = state.minimum_height;
			append_label_glyph(maximum, glyph, character, font.line_spacing(), where);
			append_label_glyph(minimum_height, glyph, character, font.line_spacing(), where);
			if (state.zero_width)
			{
				LabelPen zero_width = state.maximum;
				zero_width.x = state.zero_width_pen;
				zero_width.extent.x = 0.0f;
				append_label_glyph(zero_width, glyph, character, font.line_spacing(), where);
				state.zero_width = zero_width.extent.x == 0.0f;
				state.zero_width_pen = zero_width.x;
			}
			state.maximum = maximum;
			state.minimum_height = minimum_height.extent.y;
		};
		const auto walk = [&](State& state, std::wstring_view words)
		{
			for (const wchar_t character : words) { append(state, character); }
		};
		const auto merge = [](State& destination, const State& candidate)
		{
			if (!candidate.present) { return; }
			if (!destination.present)
			{
				destination = candidate;
				return;
			}
			// The pen transform is monotone even with negative bearings or
			// advances. Greatest pen and ink need not come from the same value.
			destination.maximum.x = std::max(destination.maximum.x, candidate.maximum.x);
			destination.maximum.extent.x = std::max(destination.maximum.extent.x, candidate.maximum.extent.x);
			destination.maximum.extent.y = std::max(destination.maximum.extent.y, candidate.maximum.extent.y);
			destination.minimum_height = std::min(destination.minimum_height, candidate.minimum_height);
			// Among still-zero-width paths, the least pen suffices to decide
			// whether later digits or a visible suffix can keep or rescue one.
			if (candidate.zero_width &&
				(!destination.zero_width || candidate.zero_width_pen < destination.zero_width_pen))
			{
				destination.zero_width = true;
				destination.zero_width_pen = candidate.zero_width_pen;
			}
		};
		constexpr std::wstring_view token = L"{round}";
		const size_t at = text.find(token);
		State prefix;
		prefix.present = true;
		prefix.zero_width = true;
		walk(prefix, text.substr(0, at));
		const std::wstring_view suffix = text.substr(at + token.size());
		const std::wstring largest = std::to_wstring(std::numeric_limits<int>::max());
		State complete;
		for (size_t length = 1; length <= largest.size(); ++length)
		{
			const std::wstring limit = length == largest.size() ? largest : std::wstring(length, L'9');
			std::array<State, 2> states{};
			states[1] = prefix;
			for (size_t position = 0; position < length; ++position)
			{
				std::array<State, 2> next{};
				for (size_t tight = 0; tight < states.size(); ++tight)
				{
					if (!states[tight].present) { continue; }
					const wchar_t last = tight != 0 ? limit[position] : L'9';
					for (wchar_t digit = position == 0 ? L'1' : L'0'; digit <= last; ++digit)
					{
						State candidate = states[tight];
						append(candidate, digit);
						const size_t next_tight = tight != 0 && digit == last ? 1 : 0;
						merge(next[next_tight], candidate);
					}
				}
				states = next;
			}
			for (State candidate : states)
			{
				if (!candidate.present) { continue; }
				walk(candidate, suffix);
				merge(complete, candidate);
			}
		}
		if (!complete.present || complete.zero_width || complete.minimum_height <= 0.0f ||
			complete.maximum.extent.x <= 0.0f || complete.maximum.extent.y <= 0.0f)
		{
			throw std::runtime_error(where +
				" must retain finite positive measured heading bounds for every positive int round.");
		}
		return complete.maximum.extent;
	}
}
