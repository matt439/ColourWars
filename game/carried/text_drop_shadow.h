#pragma once

// Carried from Labrador v0.2.0, which removes this type on master (730dd99,
// "A game that used one should copy the class ... into its own source").
// Shape unchanged; it lives in colourwars:: so it can never be ambiguous with
// an engine type. See docs/review/engine-reports.md, HYG#25.

#include "engine/render/text.h"
#include "engine/render/colour.h"
#include "engine/math/vector2f.h"

#include <string>

namespace colourwars
{
	class TextDropShadow : public labrador::Text
	{
	public:
		TextDropShadow() = default;
		TextDropShadow(const std::wstring& text,
					const std::string& font_name,
					const mattmath::Vector2F& position,
					labrador::RenderResources* render_resources,
					const labrador::Colour& color = labrador::Colour::white,
					const labrador::Colour& shadow_color = labrador::Colour::black,
					const mattmath::Vector2F& shadow_offset = { 2.0f, 2.0f },
					float scale = 1.0f,
					float shadow_scale = 1.0f,
					float rotation = 0.0f,
					const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
					float layer_depth = 0.0f);

		// const, like every draw below it - and here const also buys correctness
		// rather than only safety. A non-const draw would not override
		// TextObject::draw(...) const at all, it would *hide* it, so a
		// TextDropShadow reached through a Text& would draw its text with no
		// shadow.
		void draw(labrador::DrawList& draw_list) const override;

		mattmath::Vector2F shadow_offset() const;
		labrador::Colour shadow_color() const;
		float shadow_scale() const;

		void set_shadow_offset(const mattmath::Vector2F& offset);
		void set_shadow_color(const labrador::Colour& color);
		void set_shadow_scale(float scale);

	private:
		mattmath::Vector2F shadow_offset_ = { 2.0f, 2.0f };
		labrador::Colour shadow_color_ = labrador::Colour::black;
		float shadow_scale_ = 1.0f;
	};
}
