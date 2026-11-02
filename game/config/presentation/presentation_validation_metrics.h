#pragma once

#include "engine/math/vector2f.h"
#include "engine/render/font.h"
#include <string>
#include <string_view>

namespace presentation_validation
{
	struct LabelPen
	{
		float x = 0.0f;
		float y = 0.0f;
		mattmath::Vector2F extent = mattmath::Vector2F::ZERO;
	};

    void append_label_glyph(LabelPen& pen, const labrador::Glyph& glyph, wchar_t character,
        float line_spacing, const std::string& where);
    void append_label_text(LabelPen& pen, const labrador::Font& font, std::wstring_view text,
        const std::string& where);
    mattmath::Vector2F round_heading_size(const labrador::Font& font, std::wstring_view text,
        const std::string& where);
}
