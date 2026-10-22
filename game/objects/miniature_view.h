#pragma once

#include "engine/collision/collision_object.h"
#include "engine/render/camera.h"
#include "engine/render/viewport.h"

#include <span>

// A single overview/monitor view. Miniature terrain strokes are chosen by
// this path, not by zoom, so the view walks the scene itself and calls
// Structure::draw_miniature, which reads the camera set here from the list.
// The borrowed lists keep Scene's scenery-before-collision order; normal
// gameplay still uses Scene::draw and its view fan-out. Drawing these views
// through Scene::draw instead is POL#28 (docs/polish-terrain.md).
void draw_miniature_view(labrador::DrawList& list,
	const labrador::Camera& camera, const labrador::Viewport& viewport,
	std::span<labrador::GameObject* const> scenery,
	std::span<labrador::CollisionObject* const> collision_objects);
