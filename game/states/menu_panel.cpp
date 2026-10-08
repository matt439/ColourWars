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
	const Colour& colour, const MenuStyle* style) :
	UiTexture(name, "sprite_sheet_1", frame, rectangle, resources, colour),
	style_(style), pixel_(this->sprite_sheet()->resolve_sprite_frame("pixel"))
{
}

void MenuIcon::draw(DrawList& list) const
{
	if (this->hidden())
	{
		return;
	}
	RectangleF tile = this->rectangle();
	tile.inflate(this->style_->icon_padding);
	this->sprite_sheet()->draw(list, this->pixel_, tile, this->style_->heading_colour);
	UiTexture::draw(list);
}
