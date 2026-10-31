#include "game/pch.h"
#include "game/objects/level_builder.h"
#include "game/presentation.h"
#include "game/drawable_text.h"

using namespace mattmath;
using namespace labrador;

LevelBuilder::LevelBuilder(ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	const Registry<LevelLoadedInfo>* level_infos,
	const GameTuning* tuning,
	const Presentation* presentation,
	ResolutionManager* resolution_manager,
	ThreadPool* thread_pool,
	const Partitioner* partitioner,
	const InputBindings* input_bindings) :
	render_resources_(render_resources),
	audio_resources_(audio_resources),
	level_infos_(level_infos),
	tuning_(tuning),
	presentation_(presentation),
	viewport_manager_(viewport_manager),
	resolution_manager_(resolution_manager),
	thread_pool_(thread_pool),
	partitioner_(partitioner),
	input_bindings_(input_bindings)
{
	this->preferences_ = preferences;
	this->team_colour_ = std::make_unique<TeamColourTools>();
	this->player_builder_ = std::make_unique<PlayerBuilder>(tuning, presentation);

	this->level_object_builder_ = std::make_unique<LevelObjectBuilder>(
		this->render_resources_, this->audio_resources_, this->preferences_,
		&tuning->paint(), &presentation->gate_permission(), &presentation->ferry_direction());
}

std::unique_ptr<Level>
	LevelBuilder::build_level(const MatchSetup& settings, TeamColourPair palette,
		OpeningMode opening, const MatchRandom& random, bool practice)
{
	if (settings.player_count() < 1 || settings.human_count() < 1 ||
		(settings.bot_count() != 0 &&
			settings.screen_layout() != static_cast<ScreenLayout>(settings.human_count() - 1)))
	{ throw std::invalid_argument("Level layout must match its human participants"); }
	const LevelLoadedInfo* load_info =
		this->level_infos_->get(settings.stage());

	const TeamColour team_colours = team_colour_->team_colours(palette,
		this->presentation_->team_palette(), random);

	// Scene additions remain pending until Level builds its opening frame.
	auto scene = std::make_unique<Scene>(this->thread_pool_,
		this->partitioner_);

	scene->set_bounds(load_info->out_of_bounds_rectangle());

	for (std::unique_ptr<GameObject>& object :
		this->level_object_builder_->build_non_collision_objects(
			load_info->non_collision_objects_json()))
	{
		scene->add(std::move(object));
	}
	for (std::unique_ptr<GameObject>& shadow :
		this->level_object_builder_->build_shadows(load_info->collision_objects_json()))
	{
		scene->add(std::move(shadow));
	}

	auto collision_objects = this->level_object_builder_->build_collision_objects(
		load_info->collision_objects_json(), team_colours);
	for (std::unique_ptr<GameObject>& shadow :
		this->level_object_builder_->build_moving_shadows(load_info->collision_objects_json(), collision_objects))
	{
		scene->add(std::move(shadow));
	}
	for (std::unique_ptr<labrador::CollisionObject>& object : collision_objects)
	{
		scene->add(std::move(object));
	}

	// Scene owns the players. Return borrowed pointers in build order so Level
	// can associate each view with its player.
	std::vector<Player*> players;
	for (std::unique_ptr<Player>& player :
		this->player_builder_->build_players(settings, load_info, team_colours,
			this->render_resources_, this->audio_resources_, this->preferences_,
			this->viewport_manager_))
	{
		players.push_back(scene->add(std::move(player)));
	}

	try
	{
		// The records are filled here, inside the try, so a display name or
		// divider that fails is reported against the stage as before.
		LevelWorld world{
			.scene = std::move(scene),
			.players = std::move(players),
			.viewport_dividers = this->level_object_builder_->build_viewport_dividers(
				this->viewport_manager_) };
		const LevelServices services{
			.tuning = this->tuning_,
			.presentation = this->presentation_,
			.render_resources = this->render_resources_,
			.audio_resources = this->audio_resources_,
			.resolution_manager = this->resolution_manager_,
			.viewport_manager = this->viewport_manager_,
			.preferences = this->preferences_,
			.input_bindings = this->input_bindings_ };
		LevelStage stage{
			.name = drawable_text::widen_drawable(load_info->display_name(),
				"the display_name of stage '" + settings.stage() + "'"),
			.camera_bounds = load_info->camera_bounds_rectangle(),
			.zoom_out_start_bounds = load_info->zoom_out_start_bounds_rectangle(),
			.zoom_out_finish_bounds = load_info->zoom_out_finish_bounds_rectangle(),
			.audio = {
				.sound_bank_name = load_info->sound_bank_name(),
				.music_name = load_info->music_name(),
				.music_volume = load_info->music_volume(),
				.end_sting_name = load_info->end_sting_name(),
				.end_sting_volume = load_info->end_sting_volume() },
			.stations = load_info->stations(),
			.lessons = practice && settings.player_count() == 1 && settings.human_count() == 1 ?
				load_info->lessons() : std::vector<LevelLesson>{} };
		LevelMatch match{
			.team_colours = team_colours,
			.opening = opening,
			.duration = settings.duration(),
			.roster = &settings,
			.random = random };
		return std::make_unique<Level>(std::move(world), services,
			std::move(stage), std::move(match));
	}
	catch (const std::invalid_argument& error)
	{
		// Object-field errors already name their source JSON. Admission combines
		// the complete Scene, roster and tuning, so identify the owning stage too.
		throw std::runtime_error("level '" + settings.stage() + "': " + error.what());
	}
}
