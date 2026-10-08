#pragma once

// Developer overlays Level draws over each pane. Plain values, present in
// every configuration; only Debug builds ever turn them on.
struct LevelOverlays
{
	// Every collision object's bounds, and the scene's out-of-bounds rectangle.
	bool collision = false;

	// The camera's scroll border, the camera bounds and the followed centre.
	bool camera = false;

	bool any() const { return this->collision || this->camera; }
};
