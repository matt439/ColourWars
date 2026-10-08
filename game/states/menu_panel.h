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

// Give dark weapon artwork a light tile independently of its ready tint.
class MenuIcon final : public labrador::UiTexture
{
public:
	MenuIcon(const std::string& name, const std::string& frame,
		const mattmath::RectangleF& rectangle, labrador::RenderResources* resources,
		const labrador::Colour& colour, const MenuStyle* style);
	void draw(labrador::DrawList& list) const override;
private:
	const MenuStyle* style_;
	labrador::SpriteSheet::frame_handle pixel_;
};
