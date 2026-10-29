#pragma once

#include "engine/ui/widget.h"

#include <string>
#include "game/carried/ui_text_drop_shadow.h"

// A UiTextDropShadow that wraps every string it is given to a width, in the
// font and at the scale it draws with (game/text_wrap.h). Wrapping is the
// widget's choice, not the text's: a page opts in by building this instead of
// the plain widget, and text_wrap::check_fits is the matching load-time check
// for whatever content it will be handed.
class WrappedTextDropShadow final : public colourwars::UiTextDropShadow
{
public:
	WrappedTextDropShadow(const std::string& name,
		const std::wstring& text,
		const std::string& font_name,
		const mattmath::Vector2F& position,
		labrador::RenderResources* render_resources,
		float max_width,
		const labrador::Colour& color,
		const labrador::Colour& shadow_color,
		const mattmath::Vector2F& shadow_offset);

	// Wraps, then sets. text() therefore returns the wrapped string, with the
	// breaks this widget added; authored_text() is what it was given.
	void set_text(const std::wstring& text) override;
	// A new scale changes every measured width, so the authored string is
	// wrapped again at it.
	void set_scale(float scale) override;

	const std::wstring& authored_text() const;

	float max_width() const;

private:
	labrador::RenderResources* render_resources_ = nullptr;
	std::wstring authored_text_;
	float max_width_ = 0.0f;
};
