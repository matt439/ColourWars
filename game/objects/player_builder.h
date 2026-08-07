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

	std::unique_ptr<std::vector<std::unique_ptr<Player>>> build_players(
		const MenuLevelSettings& settings,
		const LevelLoadedInfo* load_info,
		TeamColour team_colours,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const artattack::ViewportManager* viewport_manager,
		const float* dt) const;
private:
	std::unique_ptr<WeaponBuilder> weapon_builder_ = nullptr;
};
