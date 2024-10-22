#ifndef PLAYERBUILDER_H
#define PLAYERBUILDER_H

#include "Player.h"
#include "MenuLevelSettings.h"
#include "TeamColourTools.h"
#include "WeaponBuilder.h"
#include "ViewportManager.h"

class PlayerBuilder
{
public:
	PlayerBuilder();

	std::unique_ptr<std::vector<std::unique_ptr<Player>>> build_players(
		const MenuLevelSettings& settings,
		const LevelLoadedInfo* load_info,
		TeamColour team_colours,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const ViewportManager* viewport_manager,
		const float* dt) const;
private:
	std::unique_ptr<WeaponBuilder> _weapon_builder = nullptr;
};

#endif // !PLAYERBUILDER_H
