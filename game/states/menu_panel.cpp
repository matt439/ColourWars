#include "game/pch.h"
#include "game/states/menu_panel.h"

using namespace labrador;
using namespace mattmath;

MenuPanel::MenuPanel(const std::string& name, const RectangleF& rectangle,
	RenderResources* resources, const Colour& colour, const MenuStyle* style) :
	UiTexture(name, "sprite_sheet_1", "pixel", rectangle, resources, colour),
	style_(style)
{
}

void MenuPanel::draw(DrawList& list) const
{
	if (this->hidden())
	{
		return;
	}
	const MenuStyle& style = *this->style_;
	const SpriteSheet* sheet = this->sprite_sheet();
	RectangleF shadow = this->rectangle();
	shadow.offset(style.panel_shadow_offset);
	sheet->draw(list, this->frame(), shadow, style.shadow_colour);
	RectangleF edge = this->rectangle();
	edge.inflate(style.rule_height);
	sheet->draw(list, this->frame(), edge, style.panel_edge_colour);
	UiTexture::draw(list);
	sheet->draw(list, this->frame(), RectangleF(this->rectangle().position(),
		Vector2F(this->rectangle().width, style.accent_width)), style.highlight);
}

MenuIcon::MenuIcon(const std::string& name, const std::string& frame,
	const RectangleF& rectangle, RenderResources* resources,
	const Colour& colour, const MenuStyle* style, const std::string& sheet_name) :
	UiTexture(name, sheet_name, frame, rectangle, resources, colour),
	style_(style), pixel_(this->sprite_sheet()->resolve_sprite_frame("pixel"))
{
}

void MenuIcon::set_sprite_frame(const std::string& frame, const std::string& paint_frame,
	const std::string& supply_strip)
{
	UiTexture::set_sprite_frame(frame);
	this->paint_frame_ = paint_frame.empty() ? SpriteSheet::frame_handle{} :
		this->sprite_sheet()->resolve_sprite_frame(paint_frame);
	this->supply_strip_ = supply_strip.empty() ? SpriteSheet::strip_handle{} :
		this->sprite_sheet()->resolve_animation_strip(supply_strip);
}

void MenuIcon::set_paint_colour(const Colour& colour)
{
	this->paint_colour_ = colour;
}

void MenuIcon::draw(DrawList& list) const
{
	if (this->hidden())
	{
		return;
	}
	RectangleF tile = this->rectangle();
	tile.inflate(this->style_->icon_padding);
	RectangleF border = tile;
	border.inflate(this->style_->rule_height);
	const SpriteSheet* sheet = this->sprite_sheet();
	sheet->draw(list, this->pixel_, border, this->colour());
	sheet->draw(list, this->pixel_, tile, this->style_->heading_colour);

	const RectangleI& source = sheet->sprite_frame(this->frame()).source_rectangle();
	const Vector2F source_size(static_cast<float>(source.width),
		static_cast<float>(source.height));
	const float scale = std::min(this->rectangle().width / source_size.x,
		this->rectangle().height / source_size.y);
	const Vector2F fitted_size = source_size * scale;
	const RectangleF artwork(this->rectangle().center() - fitted_size / 2.0f,
		fitted_size);
	TextureObject::draw_with(list, artwork, this->frame(), Colour::white,
		this->origin(), this->flip(), this->draw_rotation());
	if (this->paint_frame_.valid())
	{
		TextureObject::draw_with(list, artwork, this->paint_frame_, this->paint_colour_,
			this->origin(), this->flip(), this->draw_rotation());
	}
	if (this->supply_strip_.valid())
	{
		const AnimationStrip& supply = sheet->animation_strip(this->supply_strip_);
		sheet->draw(list, supply.frame_rect(supply.frame_count() - 1), artwork,
			this->paint_colour_, this->draw_rotation(), this->origin(), this->flip());
	}
}
