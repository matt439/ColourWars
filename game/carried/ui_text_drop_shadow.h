#pragma once

// Carried from Labrador v0.2.0, which removes this type on master (730dd99,
// "A game that used one should copy the class ... into its own source").
// Shape unchanged; it lives in colourwars:: so it can never be ambiguous with
// an engine type. See docs/review/engine-reports.md, HYG#25.

#include "engine/ui/widget.h"
#include "engine/render/colour.h"
#include "engine/math/vector2f.h"
#include "game/carried/text_drop_shadow.h"

#include <string>

namespace colourwars
{
	class UiTextDropShadow : public labrador::UiWidget, public TextDropShadow
	{
	public:
		UiTextDropShadow() = default;
		UiTextDropShadow(const std::string& name,
			const std::wstring& text,
			const std::string& font_name,
			const mattmath::Vector2F& position,
			labrador::RenderResources* render_resources,
			const labrador::Colour& color = labrador::Colour::white,
			const labrador::Colour& shadow_color = labrador::Colour::black,
			const mattmath::Vector2F& shadow_offset = { 2.0f, 2.0f },
			bool hidden = false,
			float scale = 1.0f,
			float shadow_scale = 1.0f,
			float rotation = 0.0f,
			const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
			float layer_depth = 0.0f);

		void update(float dt) override;
		void draw(labrador::DrawList& draw_list) const override;
		mattmath::RectangleF bounds() const override;
		void set_colour(const labrador::Colour& colour) override;
	};
}
