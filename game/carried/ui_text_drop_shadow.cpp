#include "game/pch.h"
// Carried from Labrador v0.2.0, which removes this type on master (730dd99,
// "A game that used one should copy the class ... into its own source").
// Shape unchanged; it lives in colourwars:: so it can never be ambiguous with
// an engine type. See docs/review/engine-reports.md, HYG#25.
#include "game/carried/ui_text_drop_shadow.h"

using namespace mattmath;
using namespace labrador;

namespace colourwars
{
#pragma region UiTextDropShadow

	UiTextDropShadow::UiTextDropShadow(const std::string& name,
		const std::wstring& text,
		const std::string& font_name,
		const Vector2F& position,
		RenderResources* render_resources,
		const Colour& color,
		const Colour& shadow_color,
		const Vector2F& shadow_offset,
		bool hidden,
		float scale,
		float shadow_scale,
		float rotation,
		const Vector2F& origin,
		float layer_depth) :
		UiWidget(name, hidden),
		TextDropShadow(text, font_name, position,
			render_resources, color, shadow_color, shadow_offset,
			scale, shadow_scale, rotation, origin, layer_depth)
	{

	}

	void UiTextDropShadow::draw(DrawList& draw_list) const
	{
		if (this->hidden())
		{
			return;
		}
		this->TextDropShadow::draw(draw_list);
	}
	void UiTextDropShadow::update(float /*dt*/)
	{
		return;
	}
	RectangleF UiTextDropShadow::bounds() const
	{
		// Both draws, not just the text: the shadow is offset and can carry its
		// own scale, so it is measured at its own position rather than assumed
		// to sit inside the text's box.
		return RectangleF::union_of(
			this->text_bounds(),
			this->text_bounds_at(this->position() + this->shadow_offset(),
				this->shadow_scale()));
	}
	void UiTextDropShadow::set_colour(const Colour& colour)
	{
		this->TextDropShadow::set_colour(colour);
	}

#pragma endregion UiTextDropShadow
}
