#include "game/pch.h"
#include "game/states/wrapped_text_drop_shadow.h"
#include "game/text_wrap.h"
#include "game/carried/ui_text_drop_shadow.h"

using namespace labrador;
using namespace mattmath;

namespace
{
	// The base constructor measures the text it is given, and a virtual call
	// from it would not reach this class, so the first string is wrapped here.
	std::wstring wrap_for(RenderResources* render_resources,
		const std::string& font_name, const std::wstring& text, float max_width)
	{
		return text_wrap::wrap(*render_resources,
			render_resources->resolve_sprite_font(font_name), text, max_width);
	}
}

WrappedTextDropShadow::WrappedTextDropShadow(const std::string& name,
	const std::wstring& text,
	const std::string& font_name,
	const Vector2F& position,
	RenderResources* render_resources,
	float max_width,
	const Colour& color,
	const Colour& shadow_color,
	const Vector2F& shadow_offset) :
	colourwars::UiTextDropShadow(name,
		wrap_for(render_resources, font_name, text, max_width),
		font_name,
		position,
		render_resources,
		color,
		shadow_color,
		shadow_offset),
	render_resources_(render_resources),
	authored_text_(text),
	max_width_(max_width)
{
}

void WrappedTextDropShadow::set_text(const std::wstring& text)
{
	this->authored_text_ = text;
	colourwars::UiTextDropShadow::set_text(text_wrap::wrap(*this->render_resources_,
		this->font(), text, this->max_width_, this->scale()));
}

void WrappedTextDropShadow::set_scale(float scale)
{
	colourwars::UiTextDropShadow::set_scale(scale);
	colourwars::UiTextDropShadow::set_text(text_wrap::wrap(*this->render_resources_,
		this->font(), this->authored_text_, this->max_width_, this->scale()));
}

const std::wstring& WrappedTextDropShadow::authored_text() const
{
	return this->authored_text_;
}

float WrappedTextDropShadow::max_width() const
{
	return this->max_width_;
}
