#pragma once

#include "engine/ui/widget.h"
#include "game/presentation.h"

// The game's modal frame, shared by pause, confirmation and results.
// Geometry remains a normal UiTexture so pages can center and size it.
class MenuPanel final : public labrador::UiTexture
{
public:
	MenuPanel(const std::string& name, const mattmath::RectangleF& rectangle,
		labrador::RenderResources* resources, const labrador::Colour& colour,
		const MenuStyle* style);
	void draw(labrador::DrawList& list) const override;
private:
	const MenuStyle* style_;
};

// Fit weapon artwork inside a light tile; readiness colours the border.
class MenuIcon final : public labrador::UiTexture
{
public:
	MenuIcon(const std::string& name, const std::string& frame,
		const mattmath::RectangleF& rectangle, labrador::RenderResources* resources,
		const labrador::Colour& colour, const MenuStyle* style,
		const std::string& sheet_name = "sprite_sheet_1");
	// An empty paint name is an icon with no team-coloured region.
	void set_sprite_frame(const std::string& frame, const std::string& paint_frame = "");
	void set_paint_colour(const labrador::Colour& colour);
	void draw(labrador::DrawList& list) const override;
private:
	const MenuStyle* style_;
	labrador::SpriteSheet::frame_handle pixel_;
	labrador::SpriteSheet::frame_handle paint_frame_;
	labrador::Colour paint_colour_ = labrador::Colour::white;
};
