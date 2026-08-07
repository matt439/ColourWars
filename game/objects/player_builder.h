#ifndef PLAYERBUILDER_H
#define PLAYERBUILDER_H

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
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const ViewportManager* viewport_manager,
		const float* dt) const;
private:
	std::unique_ptr<WeaponBuilder> _weapon_builder = nullptr;
};

#endif // !PLAYERBUILDER_H
