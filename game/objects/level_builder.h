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
	LevelBuilder(artattack::ViewportManager* viewport_manager,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const artattack::Registry<LevelLoadedInfo>* level_infos,
		ID3D11SamplerState* sampler_state,
		artattack::ResolutionManager* resolution_manager,
		artattack::ThreadPool* thread_pool,
		const artattack::Partitioner* partitioner);

	std::unique_ptr<Level> build_level(const MenuLevelSettings& settings);

private:
	std::unique_ptr<TeamColourTools> team_colour_ = nullptr;
	std::unique_ptr<PlayerBuilder> player_builder_ = nullptr;
	artattack::RenderResources* render_resources_ = nullptr;
	const artattack::AudioResources* audio_resources_ = nullptr;
	const artattack::Registry<LevelLoadedInfo>* level_infos_ = nullptr;
	artattack::ViewportManager* viewport_manager_ = nullptr;
	std::unique_ptr<LevelObjectBuilder> level_object_builder_ = nullptr;
	ID3D11SamplerState* sampler_state_ = nullptr;
	artattack::ResolutionManager* resolution_manager_ = nullptr;
	artattack::ThreadPool* thread_pool_ = nullptr;
	const artattack::Partitioner* partitioner_ = nullptr;
};
