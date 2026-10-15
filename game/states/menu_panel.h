#pragma once

#include "engine/ui/widget.h"
#include "engine/ui/focus.h"
#include "game/presentation.h"

// Fit weapon artwork inside a light tile; readiness colours the border.
class MenuIcon final : public labrador::UiTexture
{
public:
	MenuIcon(const std::string& name, const std::string& frame,
		const mattmath::RectangleF& rectangle, labrador::RenderResources* resources,
		const labrador::Colour& colour, const MenuStyle* style,
		const std::string& sheet_name, const std::string& fill_frame);
	// An empty paint name is an icon with no team-coloured region.
	void set_sprite_frame(const std::string& frame, const std::string& paint_frame = "",
		const std::string& supply_strip = "");
	void set_paint_colour(const labrador::Colour& colour);
	void draw(labrador::DrawList& list) const override;
private:
	const MenuStyle* style_;
	labrador::SpriteSheet::frame_handle fill_frame_;
	labrador::SpriteSheet::frame_handle paint_frame_;
	labrador::SpriteSheet::strip_handle supply_strip_;
	labrador::Colour paint_colour_ = labrador::Colour::white;
};

// Shared Paint.exe modal artwork. The focus group and widgets are borrowed
// from the page; an optional primary card stays visible when focus moves away.
class MenuPaperArtwork final : public labrador::UiTexture
{
public:
	MenuPaperArtwork(const mattmath::RectangleF& rectangle,
		labrador::RenderResources* resources, const MenuStyle* style, const MenuPaperArtPresentation* art,
		const labrador::FocusGroup* focus, const labrador::UiWidget* primary = nullptr);
	void draw(labrador::DrawList& list) const override;
private:
	void fill(labrador::DrawList& list, const mattmath::RectangleF& rectangle,
		const labrador::Colour& colour) const;
	void draw_card(labrador::DrawList& list, const mattmath::RectangleF& bounds,
		const labrador::Colour& colour) const;
	const MenuPaperArtPresentation* art_;
	const labrador::FocusGroup* focus_;
	const labrador::UiWidget* primary_;
};
