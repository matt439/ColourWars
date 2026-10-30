#pragma once

#include "engine/render/render_resources.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/text_encoding.h"
#include "game/objects/player.h"
#include "engine/render/colour.h"
#include "game/presentation_match.h"


class DebugText
{
public:
	// The presentation is borrowed and outlives every level: main() owns it.
	DebugText(labrador::RenderResources* render_resources,
		const labrador::ResolutionManager* resolution_manager,
		const DebugOverlayPresentation* look);

	void draw_debug_info(labrador::DrawList& draw_list,
		const Player* player, int num_projectiles, float simulation_dt, double frame_ms) const;
	static std::wstring frame_rate_text(double frame_ms);

private:
	labrador::RenderResources* render_resources() const;

	// Borrowed services; they outlive this overlay.
	labrador::RenderResources* render_resources_ = nullptr;
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	const DebugOverlayPresentation* look_ = nullptr;

	// Resolved at construction to avoid font-name lookup in each view.
	labrador::FontHandle font_;
};
