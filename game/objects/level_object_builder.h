#ifndef LEVELOBJECTBUILDER_H
#define LEVELOBJECTBUILDER_H

#include "game/objects/i_collision_game_object.h"
#include <memory>
#include "rapidjson/document.h"
#include "game/objects/structure_paintable.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/visual.h"
#include "engine/render/viewport_manager.h"

class LevelObjectBuilder
{
public:
	LevelObjectBuilder(RenderResources* render_resources,
		const AudioResources* audio_resources,
		const float* dt);

	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> 
		build_collision_objects(const rapidjson::Value& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> 
		build_non_collision_objects(const rapidjson::Value& json) const;

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> 
		build_viewport_dividers(const ViewportManager* viewport_manager) const;

private:
	RenderResources* _render_resources = nullptr;
	const AudioResources* _audio_resources = nullptr;
	const float* _dt = nullptr;

	std::unique_ptr<ICollisionGameObject>
		build_collision_object(const rapidjson::Value& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<IGameObject>
		build_non_collision_object(const rapidjson::Value& json) const;
};

#endif // !LEVELOBJECTBUILDER_H
