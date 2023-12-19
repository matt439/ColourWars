#ifndef PLAYERBUILDER_H
#define PLAYERBUILDER_H

#include "Player.h"
#include "MenuLevelSettings.h"
#include "TeamColour.h"
#include "level_stage.h"
//#include "Projection.h"
#include "ResolutionManager.h"
//#include "Viewport.h"
#include "WeaponBuilder.h"
//#include "SimpleMath.h"
#include "ViewportManager.h"
#include "MattMath.h"

class PlayerBuilder
{
private:
	//std::unique_ptr<TeamColour> _team_colours = nullptr;
	//Projection _projection;
	//std::unique_ptr<ViewportGenerator> _viewport_generator = nullptr;
	//ViewportManager* _viewport_manager = nullptr;
	std::unique_ptr<WeaponBuilder> _weapon_builder = nullptr;
	//DirectX::SimpleMath::Vector2 get_player_start_pos(
	//	level_stage stage, int player_num);
	//DirectX::SimpleMath::Vector2 get_player_respawn_pos(
	//	level_stage stage, int player_num);
	//MattMath::Vector2F get_player_size();
public:
	PlayerBuilder();//ViewportManager* viewport_manager);

	//std::shared_ptr<std::vector<Player>> build_players(
	//	const MenuLevelSettings& settings,
	//	team_colour team_colours,
	//	const ResolutionManager* resolution,
	//	const std::vector<MattMath::Vector2F>& team_a_spawns,
	//	const std::vector<MattMath::Vector2F>& team_b_spawns);


	std::unique_ptr<std::vector<std::unique_ptr<Player>>> build_players(
		const MenuLevelSettings& settings,
		const LevelLoadedInfo* load_info,
		team_colour team_colours,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		ViewportManager* viewport_manager,
		const float* dt);
};

#endif // !PLAYERBUILDER_H
