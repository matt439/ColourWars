#pragma once

#include "engine/render/render_resources.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/text_encoding.h"
#include "game/objects/player.h"
#include "engine/render/colour.h"
#include "game/presentation.h"


class DebugText
{
public:
	// The presentation is borrowed and outlives every level: main() owns it.
	DebugText(labrador::RenderResources* render_resources,
		const labrador::ResolutionManager* resolution_manager,
		const DebugOverlayPresentation* look);

	void draw_debug_info(labrador::DrawList& draw_list,
		const Player* player, int num_projectiles, float dt) const;

private:
	labrador::RenderResources* render_resources() const;

	// Borrowed, not owned - see InterfaceGameplay.
	labrador::RenderResources* render_resources_ = nullptr;
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	const DebugOverlayPresentation* look_ = nullptr;

	// Resolved once, like every other font name. The debug overlay is drawn
	// per player per frame when it is on at all.
	labrador::FontHandle font_;
};
