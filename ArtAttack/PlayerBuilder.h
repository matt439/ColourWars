#ifndef PLAYERBUILDER_H
#define PLAYERBUILDER_H

#include "Player.h"
#include "MenuLevelSettings.h"
#include "TeamColour.h"
#include "level_stage.h"
#include "ResolutionManager.h"
#include "WeaponBuilder.h"
#include "ViewportManager.h"
#include "MattMath.h"

class PlayerBuilder
{
public:
	PlayerBuilder();

	std::unique_ptr<std::vector<std::unique_ptr<Player>>> build_players(
		const MenuLevelSettings& settings,
		const LevelLoadedInfo* load_info,
		team_colour team_colours,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		ViewportManager* viewport_manager,
		const float* dt);
private:
	std::unique_ptr<WeaponBuilder> _weapon_builder = nullptr;
};

#endif // !PLAYERBUILDER_H
