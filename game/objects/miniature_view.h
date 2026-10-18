#pragma once

#include "engine/collision/collision_object.h"
#include "engine/render/camera.h"
#include "engine/render/viewport.h"

#include <span>

// A single overview/monitor view. Scene cannot pass its camera into an
// object's draw(), so these client views supply it explicitly for terrain
// strokes. The borrowed lists keep Scene's scenery-before-collision order;
// normal gameplay still uses Scene::draw and its view fan-out.
void draw_miniature_view(labrador::DrawList& list,
	const labrador::Camera& camera, const labrador::Viewport& viewport,
	std::span<labrador::GameObject* const> scenery,
	std::span<labrador::CollisionObject* const> collision_objects);
