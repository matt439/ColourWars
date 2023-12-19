#include "pch.h"
#include "LevelBuilder.h"

using namespace DirectX;
using namespace MattMath;
using namespace rapidjson;

LevelBuilder::LevelBuilder(ViewportManager* viewport_manager,
	const float* dt,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	ID3D11SamplerState* sampler_state,
	ResolutionManager* resolution_manager) :
	_viewport_manager(viewport_manager),
	_resource_manager(resource_manager),
	_sprite_batch(sprite_batch),
	_dt(dt),
	_sampler_state(sampler_state),
	_resolution_manager(resolution_manager)
{
	this->_team_colour = std::make_unique<TeamColour>();
	//this->_tile_builder = std::make_unique<TileBuilder>();
	//this->_player_builder = std::make_unique<PlayerBuilder>(viewport_manager);
	this->_player_builder = std::make_unique<PlayerBuilder>();

	this->_level_object_builder = std::make_unique<LevelObjectBuilder>(
		this->_resource_manager, this->_sprite_batch, this->_dt);
}

std::unique_ptr<Level>
	LevelBuilder::build_level(const MenuLevelSettings& settings)
{
	const LevelLoadedInfo* load_info =
		this->_resource_manager->get_level_info(settings.get_stage());

	const team_colour team_colours = this->_team_colour->generate_random_team_colour();

	const Value& collision_objects_json =
		load_info->get_collision_objects_json();

	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> collision_objects =
		this->_level_object_builder->build_collision_objects(collision_objects_json, team_colours);

	const Value& non_collision_objects_json =
		load_info->get_non_collision_objects_json();

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> non_collision_objects =
		this->_level_object_builder->build_non_collision_objects(non_collision_objects_json);

	std::unique_ptr<std::vector<std::unique_ptr<Player>>> players =
		//this->build_players(settings, load_info, team_colours);
		this->_player_builder->build_players(settings, load_info, team_colours,
			this->_sprite_batch, this->_resource_manager, this->_viewport_manager,
			this->_dt);

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> viewport_dividers =
		this->_level_object_builder->build_viewport_dividers(this->_viewport_manager);


	return std::make_unique<Level>(
		std::move(non_collision_objects),
		std::move(collision_objects),
		std::move(players),
		std::move(viewport_dividers),
		settings.get_stage(),
		team_colours,
		//load_info->get_camera_bounds_border(),
		load_info->get_out_of_bounds_rectangle(),
		load_info->get_camera_bounds_rectangle(),
		load_info->get_zoom_out_start_bounds_rectangle(),
		load_info->get_zoom_out_finish_bounds_rectangle(),
		load_info->get_team_a_spawns(),
		load_info->get_team_b_spawns(),
		load_info->get_sound_bank_name(),
		load_info->get_music_name(),
		load_info->get_music_volume(),
		this->_dt,
		this->_sprite_batch,
		this->_sampler_state,
		load_info->get_level_name(),
		this->_resolution_manager,
		this->_viewport_manager,
		this->_resource_manager);
}

//std::unique_ptr<std::vector<std::unique_ptr<Player>>> LevelBuilder::build_players(
//	const MenuLevelSettings& settings,
//	const LevelLoadedInfo* load_info,
//	team_colour team_colours)
//	//const ResolutionManager* resolution)
//{
//	std::unique_ptr<std::vector<std::unique_ptr<Player>>> players =
//		std::make_unique<std::vector<std::unique_ptr<Player>>>();
//
//	const std::vector<Vector2F> team_a_spawns = load_info->get_team_a_spawns();
//	const std::vector<Vector2F> team_b_spawns = load_info->get_team_b_spawns();
//
//	int i = 0;
//	for (auto& setting : settings.get_player_settings())
//	{
//		Vector2F position = Vector2F::ZERO;
//		if (setting.team == player_team::A)
//		{
//			position = team_a_spawns[i];
//		}
//		else
//		{
//			position = team_b_spawns[i];
//		}
//		RectangleF rect = RectangleF(position, player_consts::DEFAULT_SIZE);
//
//		std::unique_ptr<Player> player = std::make_unique<Player>(
//			rect,
//			player_consts::DEFAULT_SHEET,
//			player_consts::DEFAULT_ANIMATION_STRIP,
//			this->_sprite_batch,
//			this->_resource_manager,
//			setting.num,
//			setting.team,
//			setting.weapon,
//			wep_type::GRENADE_STD,
//			this->_team_colour->
//				get_team_colour(team_colours, setting.team),
//			this->_viewport_manager->
//				get_player_viewport(setting.num),
//			this->_dt);
//
//		players->push_back(std::move(player));
//
//		//weapon_info w_info;
//		//w_info.player_num = info.player_num;
//		//w_info.team = info.team;
//		//w_info.team_colour = info.team_colour;
//		//w_info.type = info.primary;
//
//		//(*players)[i].set_primary(this->_weapon_builder->build_weapon(w_info));
//
//		i++;
//	}
//	return players;
//}


//using namespace level_consts;
//
//LevelBuilder::LevelBuilder(ViewportManager* viewport_manager,
//    ResourceManager* resource_manager) :
//    _resource_manager(resource_manager)
//{
//	this->_team_colour = std::make_unique<TeamColour>();
//	this->_tile_builder = std::make_unique<TileBuilder>();
//	this->_player_builder = std::make_unique<PlayerBuilder>(viewport_manager);
//}
//
//std::unique_ptr<Level> LevelBuilder::build_level(
//    const MenuLevelSettings& settings,
//    const ResolutionManager* resolution_manager)
//{
//    level_stage stage = settings.get_stage();
//    const LevelLoadedInfo* level_loaded_info =
//        this->_resource_manager->get_level_info(stage);
//    
//    std::vector<std::vector<tile_type>> tiles =
//		this->load_tile_data(level_loaded_info->get_tile_types());
//
//    auto team_a_spawns = level_loaded_info->get_team_a_spawns();
//    auto team_b_spawns = level_loaded_info->get_team_b_spawns();
//
//    level_info info;
//    info.team_colours = this->_team_colour->generate_random_team_colour();
//    info.tiles = this->_tile_builder->build_tiles(tiles, settings, info.team_colours);
//    info.timer = TIMER;
//    info.players =
//        this->_player_builder->build_players(settings,
//        info.team_colours, resolution_manager,
//        team_a_spawns, team_b_spawns);
//    info.stage = settings.get_stage();
//    info.camera_boundary_border =
//        level_loaded_info->get_camera_bounds_border();
//
//	std::unique_ptr<Level> level = std::make_unique<Level>(info);
//	return level;
//}
//
//std::vector<std::vector<tile_type>> LevelBuilder::load_tile_data(
//    const std::vector<std::string>& level_string)
//{
//    std::vector<std::vector<tile_type>> tiles;
//    //int level_string_size = static_cast<int>(level_string.size());
//    for (size_t i = 0; i < level_string.size(); i++)
//    {
//        std::vector<tile_type> line;
//        //int line_size = static_cast<int>(level_string[i].size());
//        for (size_t j = 0; j < level_string[i].size(); j++)
//        {
//            char symbol = level_string[i].at(j);
//            tile_type type = this->convert_char_to_type(symbol);
//            line.push_back(type);
//        }
//        tiles.push_back(line);
//    }
//    return tiles;
//}
//
//tile_type LevelBuilder::convert_char_to_type(char input)
//{
//    switch (input)
//    {
//    case '.':
//        return tile_type::SKY;
//    case 'C':
//        return tile_type::CLOUD;
//    case 'D':
//        return tile_type::DIRT;
//    case 'G':
//        return tile_type::GRASS;
//    default:
//        return tile_type::SKY;
//    }
//}