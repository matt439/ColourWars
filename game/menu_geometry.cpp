#include "game/pch.h"
#include "game/menu_geometry.h"

using namespace mattmath;
using menu_consts::DESIGN_RESOLUTION;

RectangleF menu_geometry::design_area()
{
	return RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION);
}

float menu_geometry::centred(float size, float extent)
{
	return (extent - size) / 2.0f;
}

float menu_geometry::centred_x(float width)
{
	return centred(width, DESIGN_RESOLUTION.x);
}

RectangleF menu_geometry::footer_paper(const Vector2F& box, float footer_y)
{
	return RectangleF((Vector2F(DESIGN_RESOLUTION.x, footer_y) - box) / 2.0f, box);
}

RectangleF menu_geometry::screen_paper(const Vector2F& box)
{
	return RectangleF((DESIGN_RESOLUTION - box) / 2.0f, box);
}

RectangleF menu_geometry::paper_interior(const RectangleF& paper, const MenuPaperArtPresentation& art)
{
	const float edge = art.keyline_width * 3.0f;
	return RectangleF(paper.position() + Vector2F(edge),
		paper.size() - Vector2F(edge * 2.0f + art.shadow_offset));
}

float menu_geometry::paper_text_right(const RectangleF& paper, const MenuPaperArtPresentation& art)
{
	return paper.right() - art.shadow_offset - art.focus_padding.x;
}

RectangleF menu_geometry::footer_band(const MenuStyle& style)
{
	return RectangleF(0.0f, style.footer_y,
		DESIGN_RESOLUTION.x, DESIGN_RESOLUTION.y - style.footer_y);
}

RectangleF menu_geometry::footer_rule(const MenuStyle& style)
{
	return RectangleF(style.brand_position.x, style.footer_y,
		DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height);
}

MenuPaperArtPresentation menu_geometry::setup_art(const MainMenuPresentation& main)
{
	MenuPaperArtPresentation art = main.art;
	art.focus_padding = main.setup_focus_padding;
	art.shadow_offset = main.setup_shadow_offset;
	return art;
}

MenuPaperArtPresentation menu_geometry::display_art(const PauseMenuPresentation& pause,
	const MainMenuPresentation& main)
{
	MenuPaperArtPresentation art = pause.art;
	art.focus_padding = main.setup_focus_padding;
	art.shadow_offset = main.setup_shadow_offset;
	return art;
}

float menu_geometry::settings_action_width(const ControlsSettingsPresentation& look, float x)
{
	return look.controls_position.x - look.art.focus_padding.x * 2.0f - look.art.shadow_offset - x;
}

RectangleF menu_geometry::results_fill_box(const ResultsMenuPresentation& look)
{
	return RectangleF(Vector2F(centred_x(look.fill_box_size.x),
		screen_paper(look.box_size).y + look.fill_box_y_offset), look.fill_box_size);
}
