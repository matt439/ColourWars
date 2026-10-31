#include "game/pch.h"
#include "game/objects/cursor_marker_overlay.h"
#include "game/objects/level.h"
#include "game/objects/paper_card.h"
#include "game/objects/crude_drawing.h"
#include "game/presentation.h"
#include "engine/render/renderer.h"
#include "engine/render/resolution_manager.h"

using namespace mattmath;
using namespace labrador;

CursorMarkerOverlay::CursorMarkerOverlay(const Presentation* presentation,
	const ResolutionManager* resolution_manager) :
	presentation_(presentation),
	resolution_manager_(resolution_manager)
{
}

void CursorMarkerOverlay::set_pointer(std::optional<Vector2F> window_point)
{
	this->pointer_ = window_point;
}

bool CursorMarkerOverlay::visible(bool keyboard_and_mouse_seat) const
{
	if (!this->presentation_->hud().cursor_marker.enabled || !this->pointer_)
	{
		return false;
	}
	const Vector2F size = this->resolution_manager_->resolution_vec();
	const Vector2F at = *this->pointer_;
	if (at.x < 0.0f || at.y < 0.0f || at.x >= size.x || at.y >= size.y) { return false; }
	return keyboard_and_mouse_seat;
}

void CursorMarkerOverlay::draw(DrawList& list, const PaperCard& paper) const
{
	const Vector2F at = *this->pointer_;
	const SpriteSheet* sheet = paper.sheet();
	list.set_filter(TextureFilter::point);
	for (const CursorMarkerFrame& frame : Level::cursor_marker_frames(this->presentation_->hud().cursor_marker,
		at, this->resolution_manager_->resolution_vec().y))
	{
		const RectangleF& box = frame.box;
		const float width = frame.width;
		const Colour tint = CrudeDrawing::premultiplied(frame.colour);
		sheet->draw(list, paper.fill(), RectangleF(box.x, box.y, box.width, width), tint);
		sheet->draw(list, paper.fill(), RectangleF(box.x, box.bottom() - width, box.width, width), tint);
		sheet->draw(list, paper.fill(), RectangleF(box.x, box.y + width, width, box.height - width * 2.0f), tint);
		sheet->draw(list, paper.fill(), RectangleF(box.right() - width, box.y + width, width, box.height - width * 2.0f), tint);
	}
}
