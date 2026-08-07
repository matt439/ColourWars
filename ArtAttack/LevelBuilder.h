#ifndef LEVELBUILDER_H
#define LEVELBUILDER_H

#include "Level.h"
#include "MenuLevelSettings.h"
#include "TeamColour.h"
#include "PlayerBuilder.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/render_resources.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "LevelLoadedInfo.h"
#include "LevelObjectBuilder.h"
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
	std::unique_ptr<TeamColourTools> _team_colour = nullptr;
	std::unique_ptr<PlayerBuilder> _player_builder = nullptr;
	RenderResources* _render_resources = nullptr;
	const AudioResources* _audio_resources = nullptr;
	const Registry<LevelLoadedInfo>* _level_infos = nullptr;
	const float* _dt = nullptr;
	ViewportManager* _viewport_manager = nullptr;
	std::unique_ptr<LevelObjectBuilder> _level_object_builder = nullptr;
	ID3D11SamplerState* _sampler_state = nullptr;
	ResolutionManager* _resolution_manager = nullptr;
	ThreadPool* _thread_pool = nullptr;
	const Partitioner* _partitioner = nullptr;
};

#endif // !LEVELBUILDER_H
