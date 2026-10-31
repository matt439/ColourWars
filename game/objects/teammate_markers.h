#pragma once

#include "game/objects/teammate_marker.h"

#include <cstddef>
#include <vector>

namespace labrador
{
	class DrawList;
	class ViewportManager;
}
class Player;
class PlayerBadge;
struct PlaySettings;
struct HudPresentation;

// ROAD#18's edge chips for one match (HYG#40): the TeammateMarker leaf and,
// by human pane then participant, how long each teammate's body has been out
// of that pane's view. It borrows the HUD presentation, the viewport manager
// and the preferences from the owning Level's services and reads them on
// every call, so a `tune` or a Reduce effects change shows at once. The
// players and the pane-to-participant map are Level's, passed in per call.
class TeammateMarkers
{
public:
	// Every age starts at zero for `views` human panes and `players`
	// participants.
	TeammateMarkers(const HudPresentation* hud, labrador::RenderResources* render_resources,
		const labrador::ViewportManager* viewport_manager, const PlaySettings* preferences,
		std::size_t views, std::size_t players);
	TeammateMarkers(const TeammateMarkers&) = delete;
	TeammateMarkers& operator=(const TeammateMarkers&) = delete;

	// Advances each age by a live tick, holding it at the fade's end, and
	// resets it while the teammate is dead or in view.
	void update(float dt, const std::vector<Player*>& players,
		const std::vector<int>& view_participants);
	// See Level::teammate_markers; `live` is whether the match is in live
	// play, outside which there are none.
	std::vector<TeammateMarkerPlacement> markers(int view, const std::vector<Player*>& players,
		const std::vector<int>& view_participants, bool live) const;
	void draw(labrador::DrawList& draw_list, const TeammateMarkerPlacement& placement,
		const labrador::Colour& team_colour, const PlayerBadge& badge) const;

private:
	// The HUD's scale for human pane `view`, as InterfaceGameplay computes it.
	float hud_scale(int view) const;

	const HudPresentation* hud_ = nullptr;
	const labrador::ViewportManager* viewport_manager_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
	TeammateMarker marker_;
	// By human pane, then participant: seconds that teammate's body has been
	// outside the pane's view. Presentation only, advanced by live ticks.
	std::vector<float> age_;
};
