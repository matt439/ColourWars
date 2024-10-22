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
	_resource_manager(resource_manager),
	_sprite_batch(sprite_batch),
	_dt(dt),
	_viewport_manager(viewport_manager),
	_sampler_state(sampler_state),
	_resolution_manager(resolution_manager)
{
	this->_team_colour = std::make_unique<TeamColour>();
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
