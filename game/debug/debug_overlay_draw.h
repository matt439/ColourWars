#pragma once

#include "game/debug/dev_tools.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/level_overlays.h"
#include "engine/math/segment.h"
#include "engine/math/shape.h"
#include "engine/render/sprite_sheet.h"
#include "engine/scene/scene.h"
#include <array>

class Player;

// The per-pane developer overlays (overlay.collision, overlay.camera), drawn
// by Level::draw_player_overlay on the worker that owns the pane. Read-only
// and text-free, so it is safe there. The body is Debug-only; in Release this
// is an inline no-op and Level never turns the overlays on.
namespace debug_overlay
{
#if COLOURWARS_DEV_TOOLS
	// What overlay.collision draws for one object, split from the drawing so
	// the debug_console probe can check it without a device.
	//
	// The colour for a tag; a player's is its team colour instead, which
	// draw_pane reads from the object. Every structure tag has one of its own,
	// and anything else (the projectiles) is yellow.
	labrador::Colour collision_colour(CollisionObjectType type);
	// The four edges outlined: a rotated rectangle's own (a fixed slab along
	// its slope), any other shape's bounding box.
	std::array<labrador::Segment, 4> collision_outline(const labrador::Shape& shape);

	void draw_pane(labrador::DrawList& list, const LevelOverlays& overlays,
		const labrador::Scene& scene, const labrador::Scene::View& view,
		const Player& player, const labrador::RectangleF& camera_bounds,
		const labrador::SpriteSheet& sheet, labrador::SpriteSheet::frame_handle fill);
#else
	inline void draw_pane(labrador::DrawList&, const LevelOverlays&,
		const labrador::Scene&, const labrador::Scene::View&,
		const Player&, const labrador::RectangleF&,
		const labrador::SpriteSheet&, labrador::SpriteSheet::frame_handle)
	{
	}
#endif
}
