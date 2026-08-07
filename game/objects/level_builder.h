#pragma once

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
#include "rapidjson/document.h"
#include "engine/render/viewport_manager.h"
#include "engine/collision/partitioner.h"

class LevelBuilder
{
public:
	LevelBuilder(ViewportManager* viewport_manager,
		const float* dt,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const Registry<LevelLoadedInfo>* level_infos,
		ID3D11SamplerState* sampler_state,
		ResolutionManager* resolution_manager,
		ThreadPool* thread_pool,
		const Partitioner* partitioner);

	std::unique_ptr<Level> build_level(const MenuLevelSettings& settings);

private:
	std::unique_ptr<TeamColourTools> team_colour_ = nullptr;
	std::unique_ptr<PlayerBuilder> player_builder_ = nullptr;
	RenderResources* render_resources_ = nullptr;
	const AudioResources* audio_resources_ = nullptr;
	const Registry<LevelLoadedInfo>* level_infos_ = nullptr;
	const float* dt_ = nullptr;
	ViewportManager* viewport_manager_ = nullptr;
	std::unique_ptr<LevelObjectBuilder> level_object_builder_ = nullptr;
	ID3D11SamplerState* sampler_state_ = nullptr;
	ResolutionManager* resolution_manager_ = nullptr;
	ThreadPool* thread_pool_ = nullptr;
	const Partitioner* partitioner_ = nullptr;
};
