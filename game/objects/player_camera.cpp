#include "game/pch.h"
#include "game/objects/player_camera.h"
#include "game/presentation.h"

#include <cmath>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

Camera PlayerCamera::follow(const Vector2F& player_center,
	const Viewport& viewport, float screen_height,
	const CameraPresentation& framing, const Camera& previous,
	const RectangleF& bounds) const
{
	// Use the same whole-pixel pane as the renderer, including odd window sizes.
	const Viewport pixels(viewport.pixel_rect());
	if (!std::isfinite(screen_height) || screen_height <= 0.0f ||
		!std::isfinite(framing.reference_height) || framing.reference_height <= 0.0f ||
		pixels.width <= 0.0f || pixels.height <= 0.0f)
	{
		throw std::invalid_argument("PlayerCamera needs a positive viewport and reference height.");
	}

	Camera scaled = previous;
	// Preserve the 1080p baseline across resolution changes. Other aspect ratios
	// keep the vertical extent and show more/less horizontal world, without bars.
	scaled.scale = screen_height / framing.reference_height;
	const Vector2F world_size = scaled.visible_rectangle(pixels).size();
	Camera result = this->tools_.calculate_camera(player_center,
		world_size, scaled, bounds);

	// A view wider/taller than the arena cannot be clamped inside it. Centre that
	// axis instead of alternating between opposite edges on successive updates.
	if (world_size.x >= bounds.width)
	{
		result.translation.x = bounds.x + (bounds.width - world_size.x) / 2.0f;
	}
	if (world_size.y >= bounds.height)
	{
		result.translation.y = bounds.y + (bounds.height - world_size.y) / 2.0f;
	}
	return result;
}

Vector2F PlayerCamera::mouse_direction(const Vector2F& player_center,
	const Vector2F& cursor, const Viewport& viewport, const Camera& camera)
{
	const Vector2F pane_origin(viewport.pixel_rect().position());
	const Vector2F player_on_screen =
		camera.calculate_view_position(player_center) + pane_origin;
	const Vector2F direction = cursor - player_on_screen;
	// A positive uniform camera scale preserves angles; no aim quantization or
	// second, potentially inconsistent inverse transform is needed.
	return direction == Vector2F::ZERO ? Vector2F::ZERO : direction.normalized();
}

Camera PlayerCamera::displaced(const Camera& base, const Viewport& viewport,
	const Vector2F& offset, const RectangleF& bounds)
{
	Camera result = base;
	const Vector2F size = base.visible_rectangle(Viewport(viewport.pixel_rect())).size();
	result.translation.x = size.x >= bounds.width ?
		bounds.x + (bounds.width - size.x) / 2.0f :
		std::clamp(base.translation.x + offset.x, bounds.left(), bounds.right() - size.x);
	result.translation.y = size.y >= bounds.height ?
		bounds.y + (bounds.height - size.y) / 2.0f :
		std::clamp(base.translation.y + offset.y, bounds.top(), bounds.bottom() - size.y);
	return result;
}
