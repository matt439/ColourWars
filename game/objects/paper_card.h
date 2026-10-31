#pragma once

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "engine/render/render_resources.h"
#include "engine/render/sprite_sheet.h"

namespace labrador
{
	class DrawList;
	class ResolutionManager;
}
class Presentation;

// The match's paper card (HYG#40): the overlay sheet's fill frame, which the
// opening cards, the controls reminders, the practice lessons, the identity
// tags, the cursor marker and the developer overlays all draw with, and the
// scale every window-space card takes from the window. Every member is a
// borrow from the owning Level's services; the scale and the menu look are
// read through them on each call, so a resize or a `tune` shows at once.
class PaperCard
{
public:
	PaperCard() = default;
	// Resolves presentation.hud's sheet and fill frame, as the Level
	// constructor did; an unknown name throws from here.
	PaperCard(const Presentation* presentation,
		const labrador::ResolutionManager* resolution_manager,
		labrador::RenderResources* render_resources);

	// The window's size over presentation.match.reference_size, by the
	// tighter axis. Read live, so it follows the window.
	float scale() const;
	// The window in pixels, from the borrowed resolution manager.
	mattmath::Vector2F resolution() const;
	// A shadowed, edged card with the menu's accent rule along its top.
	void draw(labrador::DrawList& draw_list,
		const mattmath::RectangleF& rectangle, float scale) const;

	const labrador::SpriteSheet* sheet() const;
	labrador::SpriteSheet::frame_handle fill() const { return this->fill_; }

private:
	const Presentation* presentation_ = nullptr;
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;
	labrador::RenderResources::SpriteSheetHandle sheet_;
	labrador::SpriteSheet::frame_handle fill_;
};
