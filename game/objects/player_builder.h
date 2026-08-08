#pragma once

#include "game/objects/player.h"
#include "game/states/menu_level_settings.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/team_colour.h"
#include "game/objects/weapon_builder.h"
#include "engine/render/viewport_manager.h"

class PlayerBuilder
{
public:
	PlayerBuilder();

	// In the order the player settings name them, which is the order the level
	// fills its view list in - so index 0 of this vector is the player whose
	// pane is view 0.
	std::vector<std::unique_ptr<Player>> build_players(
		const MenuLevelSettings& settings,
		const LevelLoadedInfo* load_info,
		TeamColour team_colours,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const artattack::ViewportManager* viewport_manager) const;
private:
	std::unique_ptr<WeaponBuilder> weapon_builder_ = nullptr;
};
