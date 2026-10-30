#pragma once

#include "game/play_settings.h"

#include "game/objects/player.h"
#include "game/match_setup.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/team_colour.h"
#include "game/objects/weapon_builder.h"
#include "engine/render/viewport_manager.h"

class PlayerBuilder
{
public:
	// `tuning` is borrowed and outlives every player this makes.
	PlayerBuilder(const GameTuning* tuning, const Presentation* presentation);

	// Return participants in roster order. Only humans consume panes, in their
	// roster order; input slots are the independent SeatSetup::num.
	std::vector<std::unique_ptr<Player>> build_players(
		const MatchSetup& settings,
		const LevelLoadedInfo* load_info,
		TeamColour team_colours,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const labrador::ViewportManager* viewport_manager) const;
private:
	const GameTuning* tuning_ = nullptr;
	const Presentation* presentation_ = nullptr;

	std::unique_ptr<WeaponBuilder> weapon_builder_ = nullptr;
};
