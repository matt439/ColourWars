#pragma once

#include "engine/collision/collision_object.h"
#include <memory>
#include "rapidjson/document.h"
#include "game/objects/structure_paintable.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/visual.h"
#include "engine/render/viewport_manager.h"

class LevelObjectBuilder
{
public:
	LevelObjectBuilder(artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources);

	std::unique_ptr<std::vector<std::unique_ptr<artattack::CollisionObject>>> 
		build_collision_objects(const rapidjson::Value& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<std::vector<std::unique_ptr<artattack::GameObject>>> 
		build_non_collision_objects(const rapidjson::Value& json) const;

	std::unique_ptr<std::vector<std::unique_ptr<artattack::GameObject>>> 
		build_viewport_dividers(const artattack::ViewportManager* viewport_manager) const;

private:
	artattack::RenderResources* render_resources_ = nullptr;
	const artattack::AudioResources* audio_resources_ = nullptr;

	std::unique_ptr<artattack::CollisionObject>
		build_collision_object(const rapidjson::Value& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<artattack::GameObject>
		build_non_collision_object(const rapidjson::Value& json) const;
};
