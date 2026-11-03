#pragma once

#include "game/carried/camera_tools.h"

struct CameraPresentation;

// The game's framing policy around Labrador's world-space follow helper.
class PlayerCamera
{
public:
	labrador::Camera follow(const labrador::Vector2F& player_center,
		const labrador::Viewport& viewport, float screen_height,
		const CameraPresentation& framing, const labrador::Camera& previous,
		const labrador::RectangleF& bounds, float horizontal_velocity = 0.0f) const;

	// Returns zero when the cursor is exactly on the player. The caller retains
	// the last aim direction in that case. All other angles stay continuous.
	static labrador::Vector2F mouse_direction(
		const labrador::Vector2F& player_center,
		const labrador::Vector2F& cursor, const labrador::Viewport& viewport,
		const labrador::Camera& camera);

	// Render-only offset. Never feed it back into follow or mouse_direction.
	// Bound it to the arena; oversized axes retain their centred framing.
	static labrador::Camera displaced(const labrador::Camera& base,
		const labrador::Viewport& viewport, const labrador::Vector2F& offset,
		const labrador::RectangleF& bounds);

private:
	colourwars::CameraTools tools_;
};
