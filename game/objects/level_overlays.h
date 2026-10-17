#pragma once

// Developer overlays. Plain values, present in every configuration; only
// Debug builds ever turn them on.
struct LevelOverlays
{
	// Every collision object's bounds, and the scene's out-of-bounds rectangle.
	bool collision = false;

	// The camera's scroll border, the camera bounds and the followed centre.
	bool camera = false;

	// Each authored station's name (the `goto` targets) at its place in the
	// world. Not a pane overlay: the names are labrador::Label objects the
	// scene holds and culls, so they draw wherever the world does, under
	// collision objects (see Level::show_station_labels).
	bool stations = false;

	// What Level draws over each pane, after the world.
	bool any_pane() const { return this->collision || this->camera; }
};
