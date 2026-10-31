#include "game/pch.h"
#include "game/objects/teammate_markers.h"
#include "game/objects/player.h"
#include "game/play_settings.h"
#include "engine/render/viewport_manager.h"
#include <algorithm>

using namespace mattmath;
using namespace labrador;

TeammateMarkers::TeammateMarkers(const HudPresentation* hud, RenderResources* render_resources,
	const ViewportManager* viewport_manager, const PlaySettings* preferences,
	std::size_t views, std::size_t players) :
	hud_(hud),
	viewport_manager_(viewport_manager),
	preferences_(preferences),
	marker_(&hud->teammate_marker, render_resources)
{
	this->age_.assign(views * players, 0.0f);
}

float TeammateMarkers::hud_scale(int view) const
{
	const HudPresentation& look = *this->hud_;
	const Vector2F pane = this->viewport_manager_->player_viewport(view).size();
	return std::min(pane.x / look.player_identity_reference_width, pane.y / look.reference_height);
}

std::vector<TeammateMarkerPlacement> TeammateMarkers::markers(int view, const std::vector<Player*>& players,
	const std::vector<int>& view_participants, bool live) const
{
	std::vector<TeammateMarkerPlacement> markers;
	const TeammateMarkerPresentation& look = this->hud_->teammate_marker;
	const int view_count = static_cast<int>(view_participants.size());
	if (view < 0 || view >= view_count || !look.enabled || !live) { return markers; }
	const int player_count = static_cast<int>(players.size());
	const int viewer = view_participants.at(static_cast<std::size_t>(view));
	const Player& own = *players.at(static_cast<std::size_t>(viewer));
	const Vector2F pane = Viewport(this->viewport_manager_->player_viewport(view).pixel_rect()).size();
	const float scale = this->hud_scale(view);
	const bool steady = this->preferences_->reduce_effects_for(own.player_num());
	for (int participant = 0; participant < player_count; ++participant)
	{
		const Player& mate = *players.at(static_cast<std::size_t>(participant));
		if (participant == viewer || mate.team() != own.team() ||
			mate.state() != PlayerState::alive || mate.health() <= 0.0f) { continue; }
		std::optional<TeammateMarkerPlacement> placed = TeammateMarker::place(
			look, pane, own.camera(), mate.rectangle(), scale);
		if (!placed) { continue; }
		placed->participant = participant;
		const float age = this->age_[
			static_cast<std::size_t>(view * player_count + participant)];
		placed->opacity = steady || look.fade_in <= 0.0f ? 1.0f :
			std::clamp(age / look.fade_in, 0.0f, 1.0f);
		markers.push_back(*placed);
	}
	return markers;
}

void TeammateMarkers::update(float dt, const std::vector<Player*>& players,
	const std::vector<int>& view_participants)
{
	const TeammateMarkerPresentation& look = this->hud_->teammate_marker;
	const int count = static_cast<int>(players.size());
	const int view_count = static_cast<int>(view_participants.size());
	for (int view = 0; view < view_count; ++view)
	{
		const Player& own = *players.at(static_cast<std::size_t>(
			view_participants.at(static_cast<std::size_t>(view))));
		const Vector2F pane = Viewport(this->viewport_manager_->player_viewport(view).pixel_rect()).size();
		const float scale = this->hud_scale(view);
		for (int participant = 0; participant < count; ++participant)
		{
			float& age = this->age_[static_cast<std::size_t>(view * count + participant)];
			const Player& mate = *players.at(static_cast<std::size_t>(participant));
			const bool away = mate.state() == PlayerState::alive && mate.health() > 0.0f &&
				TeammateMarker::place(look, pane, own.camera(), mate.rectangle(), scale).has_value();
			// Held at the fade's end so a long absence never grows without bound.
			age = away ? std::min(age + std::max(0.0f, dt), std::max(look.fade_in, 0.0f)) : 0.0f;
		}
	}
}

void TeammateMarkers::draw(DrawList& list, const TeammateMarkerPlacement& placement,
	const Colour& team_colour, const PlayerBadge& badge) const
{
	this->marker_.draw(list, placement, team_colour, badge);
}
