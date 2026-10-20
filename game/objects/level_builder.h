#pragma once

#include "game/play_settings.h"

#include "game/objects/level.h"
#include "game/states/menu_level_settings.h"
#include "game/objects/team_colour.h"
#include "game/objects/player_builder.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/render_resources.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "engine/render/viewport_manager.h"
#include "engine/collision/partitioner.h"
#include "engine/input/mouse.h"

class LevelBuilder
{
public:
	LevelBuilder(labrador::ViewportManager* viewport_manager,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const labrador::Registry<LevelLoadedInfo>* level_infos,
		const GameTuning* tuning,
		const Presentation* presentation,
		labrador::ResolutionManager* resolution_manager,
		labrador::ThreadPool* thread_pool,
		const labrador::Partitioner* partitioner,
		const labrador::Mouse* mouse, const InputBindings* input_bindings = nullptr);

	std::unique_ptr<Level> build_level(const MenuLevelSettings& settings,
		TeamColourPair palette, OpeningMode opening);

private:
	std::unique_ptr<TeamColourTools> team_colour_ = nullptr;
	std::unique_ptr<PlayerBuilder> player_builder_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;
	const labrador::AudioResources* audio_resources_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
	const labrador::Registry<LevelLoadedInfo>* level_infos_ = nullptr;
	const GameTuning* tuning_ = nullptr;
	const Presentation* presentation_ = nullptr;
	labrador::ViewportManager* viewport_manager_ = nullptr;
	std::unique_ptr<LevelObjectBuilder> level_object_builder_ = nullptr;
	labrador::ResolutionManager* resolution_manager_ = nullptr;
	labrador::ThreadPool* thread_pool_ = nullptr;
	const labrador::Partitioner* partitioner_ = nullptr;

	// Borrowed mouse state passed to Level for camera-dependent aiming.
	const labrador::Mouse* mouse_ = nullptr;
	const InputBindings* input_bindings_ = nullptr;
};
