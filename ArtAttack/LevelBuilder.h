#ifndef LEVELBUILDER_H
#define LEVELBUILDER_H

#include "Level.h"
#include "MenuLevelSettings.h"
//#include "tile_consts.h"
//#include <memory>
//#include <vector>
//#include <string>
//#include "level_stage.h"
//#include "tile_type.h"
#include "TeamColour.h"
#include "PlayerBuilder.h"
//#include "TileBuilder.h"
//#include "screen_layout.h"
#include "ResolutionManager.h"
#include "ResourceManager.h"
#include "LevelObjectBuilder.h"
#include "rapidjson/document.h"
#include "ViewportManager.h"

class LevelBuilder
{
//private:
	std::unique_ptr<TeamColour> _team_colour = nullptr;
	std::unique_ptr<PlayerBuilder> _player_builder = nullptr;
//	std::unique_ptr<TileBuilder> _tile_builder = nullptr;
	ResourceManager* _resource_manager = nullptr;
	DirectX::SpriteBatch* _sprite_batch = nullptr;
	const float* _dt = nullptr;
	ViewportManager* _viewport_manager = nullptr;
	std::unique_ptr<LevelObjectBuilder> _level_object_builder = nullptr;
	ID3D11SamplerState* _sampler_state = nullptr;
	ResolutionManager* _resolution_manager = nullptr;

	//std::unique_ptr<std::vector<std::unique_ptr<Player>>> build_players(
	//	const MenuLevelSettings& settings,
	//	const LevelLoadedInfo* load_info,
	//	team_colour team_colours);
	

		//const ResolutionManager* resolution);
//
//	tile_type convert_char_to_type(char input);
//
//	std::vector<std::vector<tile_type>> load_tile_data(
//		const std::vector<std::string>& level_string);
public:
	LevelBuilder(ViewportManager* viewport_manager,
		const float* dt,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		ID3D11SamplerState* sampler_state,
		ResolutionManager* resolution_manager);


	std::unique_ptr<Level> build_level(const MenuLevelSettings& settings);

	//std::unique_ptr<Level> build_level(const MenuLevelSettings& settings,
	//	const ResolutionManager* resolution_manager);
};

#endif // !LEVELBUILDER_H
