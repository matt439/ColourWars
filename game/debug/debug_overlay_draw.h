#pragma once

#include "game/debug/dev_tools.h"
#include "game/objects/level_overlays.h"
#include "engine/render/sprite_sheet.h"
#include "engine/scene/scene.h"

class Player;

// The per-pane developer overlays (overlay.collision, overlay.camera), drawn
// by Level::draw_player_overlay on the worker that owns the pane. Read-only
// and text-free, so it is safe there. The body is Debug-only; in Release this
// is an inline no-op and Level never turns the overlays on.
namespace debug_overlay
{
#if COLOURWARS_DEV_TOOLS
	void draw_pane(labrador::DrawList& list, const LevelOverlays& overlays,
		const labrador::Scene& scene, const labrador::Scene::View& view,
		const Player& player, const mattmath::RectangleF& camera_bounds,
		const labrador::SpriteSheet& sheet, labrador::SpriteSheet::frame_handle fill);
#else
	inline void draw_pane(labrador::DrawList&, const LevelOverlays&,
		const labrador::Scene&, const labrador::Scene::View&,
		const Player&, const mattmath::RectangleF&,
		const labrador::SpriteSheet&, labrador::SpriteSheet::frame_handle)
	{
	}
#endif
}
