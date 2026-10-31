#include "game/pch.h"
#include "game/objects/paper_card.h"
#include "game/presentation.h"
#include "engine/render/renderer.h"
#include "engine/render/resolution_manager.h"
#include <algorithm>

using namespace mattmath;
using namespace labrador;

PaperCard::PaperCard(const Presentation* presentation,
	const ResolutionManager* resolution_manager, RenderResources* render_resources) :
	presentation_(presentation),
	resolution_manager_(resolution_manager),
	render_resources_(render_resources)
{
	this->sheet_ = render_resources->resolve_sprite_sheet(
		presentation->hud().sprite_sheet);
	this->fill_ = render_resources->sprite_sheet(this->sheet_)
		->resolve_sprite_frame(presentation->hud().fill_frame);
}

float PaperCard::scale() const
{
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const Vector2F reference = this->presentation_->match().reference_size;
	return std::min(resolution.x / reference.x, resolution.y / reference.y);
}

Vector2F PaperCard::resolution() const
{
	return this->resolution_manager_->resolution_vec();
}

const SpriteSheet* PaperCard::sheet() const
{
	return this->render_resources_->sprite_sheet(this->sheet_);
}

void PaperCard::draw(DrawList& draw_list,
	const RectangleF& rectangle, float scale) const
{
	const MenuStyle& menu = this->presentation_->menu();
	const SpriteSheet* sheet = this->sheet();
	sheet->draw(draw_list, this->fill_,
		RectangleF(rectangle.position() + menu.panel_shadow_offset * scale, rectangle.size()),
		menu.scrim_colour);
	sheet->draw(draw_list, this->fill_, rectangle, menu.panel_edge_colour);
	const float border = menu.rule_height * scale;
	sheet->draw(draw_list, this->fill_, RectangleF(
		rectangle.x + border, rectangle.y + border,
		rectangle.width - border * 2.0f, rectangle.height - border * 2.0f),
		menu.focus_panel_colour);
	sheet->draw(draw_list, this->fill_, RectangleF(
		rectangle.x + border, rectangle.y + border,
		rectangle.width - border * 2.0f, menu.accent_width * scale), menu.highlight);
}
