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

	// By value. The outer unique_ptr on all three of these was a heap
	// allocation around a type that already owns its storage and already moves
	// in constant time - and it bought a second null to check at every use
	// site, which is what `unique_ptr<vector<unique_ptr<T>>>` costs and all it
	// costs.
	std::vector<std::unique_ptr<artattack::CollisionObject>>
		build_collision_objects(const rapidjson::Value& json,
			const TeamColour& team_colours) const;

	std::vector<std::unique_ptr<artattack::GameObject>>
		build_non_collision_objects(const rapidjson::Value& json) const;

	std::vector<std::unique_ptr<artattack::GameObject>>
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
