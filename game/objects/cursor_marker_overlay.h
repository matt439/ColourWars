#pragma once

#include "engine/math/vector2f.h"

#include <optional>

namespace labrador
{
	class DrawList;
	class ResolutionManager;
}
class Presentation;
class PaperCard;

// POL#39's marquee around the mouse pointer (HYG#40): where the focused
// pointer is, whether the marquee shows there and how it is drawn. It borrows
// the presentation and the resolution manager from the owning Level's
// services and reads hud.cursor_marker and the window through them on every
// call, so a `tune` or a resize shows at once. Level::cursor_marker_frames
// stays the one place the frames are worked out; this draws them.
class CursorMarkerOverlay
{
public:
	CursorMarkerOverlay() = default;
	CursorMarkerOverlay(const Presentation* presentation,
		const labrador::ResolutionManager* resolution_manager);

	// See Level::set_pointer.
	void set_pointer(std::optional<mattmath::Vector2F> window_point);
	// Enabled, the pointer focused inside the window, and
	// `keyboard_and_mouse_seat`: a human seat uses keyboard/mouse.
	bool visible(bool keyboard_and_mouse_seat) const;
	// The frames at the pointer through the paper card's sheet and fill.
	// Only while visible: it reads the pointer unchecked.
	void draw(labrador::DrawList& draw_list, const PaperCard& paper) const;

private:
	const Presentation* presentation_ = nullptr;
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	std::optional<mattmath::Vector2F> pointer_;
};
