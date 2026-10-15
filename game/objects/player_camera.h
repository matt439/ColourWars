#pragma once

#include "engine/render/camera_tools.h"

struct CameraPresentation;

// The game's framing policy around Labrador's world-space follow helper.
class PlayerCamera
{
public:
	labrador::Camera follow(const mattmath::Vector2F& player_center,
		const labrador::Viewport& viewport, float screen_height,
		const CameraPresentation& framing, const labrador::Camera& previous,
		const mattmath::RectangleF& bounds) const;

	// Returns zero when the cursor is exactly on the player. The caller retains
	// the last aim direction in that case. All other angles stay continuous.
	static mattmath::Vector2F mouse_direction(
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& cursor, const labrador::Viewport& viewport,
		const labrador::Camera& camera);

	// Render-only offset. Never feed it back into follow or mouse_direction.
	// Bound it to the arena; oversized axes retain their centred framing.
	static labrador::Camera displaced(const labrador::Camera& base,
		const labrador::Viewport& viewport, const mattmath::Vector2F& offset,
		const mattmath::RectangleF& bounds);

private:
	labrador::CameraTools tools_;
};
