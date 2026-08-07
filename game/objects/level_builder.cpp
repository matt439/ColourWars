#include "game/pch.h"
#include "game/objects/level_builder.h"

using namespace DirectX;
using namespace mattmath;
using namespace rapidjson;

LevelBuilder::LevelBuilder(ViewportManager* viewport_manager,
	const float* dt,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const Registry<LevelLoadedInfo>* level_infos,
	ID3D11SamplerState* sampler_state,
	ResolutionManager* resolution_manager,
	ThreadPool* thread_pool,
	const Partitioner* partitioner) :
	render_resources_(render_resources),
	audio_resources_(audio_resources),
	level_infos_(level_infos),
	dt_(dt),
	viewport_manager_(viewport_manager),
	sampler_state_(sampler_state),
	resolution_manager_(resolution_manager),
	thread_pool_(thread_pool),
	partitioner_(partitioner)
{
	this->team_colour_ = std::make_unique<TeamColourTools>();
	this->player_builder_ = std::make_unique<PlayerBuilder>();

	this->level_object_builder_ = std::make_unique<LevelObjectBuilder>(
		this->render_resources_, this->audio_resources_, this->dt_);
}

std::unique_ptr<Level>
	LevelBuilder::build_level(const MenuLevelSettings& settings)
{
	const LevelLoadedInfo* load_info =
		this->level_infos_->get(level_asset_name(settings.get_stage()));

	const TeamColour team_colours = team_colour_->generate_random_team_colour();

	const Value& collision_objects_json =
		load_info->get_collision_objects_json();

	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> collision_objects =
		this->level_object_builder_->build_collision_objects(collision_objects_json, team_colours);

	const Value& non_collision_objects_json =
		load_info->get_non_collision_objects_json();

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> non_collision_objects =
		this->level_object_builder_->build_non_collision_objects(non_collision_objects_json);

	std::unique_ptr<std::vector<std::unique_ptr<Player>>> players =
		this->player_builder_->build_players(settings, load_info, team_colours,
			this->render_resources_, this->audio_resources_,
			this->viewport_manager_, this->dt_);

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> viewport_dividers =
		this->level_object_builder_->build_viewport_dividers(this->viewport_manager_);


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
		this->dt_,
		this->sampler_state_,
		load_info->get_level_name(),
		this->resolution_manager_,
		this->viewport_manager_,
		this->render_resources_,
		this->audio_resources_,
		this->thread_pool_,
		this->partitioner_);
}
