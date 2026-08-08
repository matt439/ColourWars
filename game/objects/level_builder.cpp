#include "game/pch.h"
#include "game/objects/level_builder.h"

using namespace DirectX;
using namespace mattmath;
using namespace rapidjson;
using namespace artattack;

LevelBuilder::LevelBuilder(ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const Registry<LevelLoadedInfo>* level_infos,
	ResolutionManager* resolution_manager,
	ThreadPool* thread_pool,
	const Partitioner* partitioner) :
	render_resources_(render_resources),
	audio_resources_(audio_resources),
	level_infos_(level_infos),
	viewport_manager_(viewport_manager),
	resolution_manager_(resolution_manager),
	thread_pool_(thread_pool),
	partitioner_(partitioner)
{
	this->team_colour_ = std::make_unique<TeamColourTools>();
	this->player_builder_ = std::make_unique<PlayerBuilder>();

	this->level_object_builder_ = std::make_unique<LevelObjectBuilder>(
		this->render_resources_, this->audio_resources_);
}

std::unique_ptr<Level>
	LevelBuilder::build_level(const MenuLevelSettings& settings)
{
	const LevelLoadedInfo* load_info =
		this->level_infos_->get(settings.stage());

	const TeamColour team_colours = team_colour_->generate_random_team_colour();

	const Value& collision_objects_json =
		load_info->collision_objects_json();

	std::unique_ptr<std::vector<std::unique_ptr<artattack::CollisionObject>>> collision_objects =
		this->level_object_builder_->build_collision_objects(collision_objects_json, team_colours);

	const Value& non_collision_objects_json =
		load_info->non_collision_objects_json();

	std::unique_ptr<std::vector<std::unique_ptr<GameObject>>> non_collision_objects =
		this->level_object_builder_->build_non_collision_objects(non_collision_objects_json);

	std::unique_ptr<std::vector<std::unique_ptr<Player>>> players =
		this->player_builder_->build_players(settings, load_info, team_colours,
			this->render_resources_, this->audio_resources_,
			this->viewport_manager_);

	std::unique_ptr<std::vector<std::unique_ptr<GameObject>>> viewport_dividers =
		this->level_object_builder_->build_viewport_dividers(this->viewport_manager_);


	return std::make_unique<Level>(
		std::move(non_collision_objects),
		std::move(collision_objects),
		std::move(players),
		std::move(viewport_dividers),
		team_colours,
		load_info->out_of_bounds_rectangle(),
		load_info->camera_bounds_rectangle(),
		load_info->zoom_out_start_bounds_rectangle(),
		load_info->zoom_out_finish_bounds_rectangle(),
		load_info->team_a_spawns(),
		load_info->team_b_spawns(),
		load_info->sound_bank_name(),
		load_info->music_name(),
		load_info->music_volume(),
		this->resolution_manager_,
		this->viewport_manager_,
		this->render_resources_,
		this->audio_resources_,
		this->thread_pool_,
		this->partitioner_);
}
