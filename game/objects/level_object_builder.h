#pragma once

#include "engine/assets/json.h"
#include "engine/collision/collision_object.h"
#include <memory>
#include "game/objects/structure_paintable.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/visual.h"
#include "engine/render/viewport_manager.h"

class LevelObjectBuilder
{
public:
	// The paint tuning is borrowed and outlives every level: main() owns it.
	// A paintable structure divides itself into tiles at construction, and
	// how finely is a balance number rather than a property of the level file.
	LevelObjectBuilder(labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PaintTuning* paint);

	// By value. The outer unique_ptr on all three of these was a heap
	// allocation around a type that already owns its storage and already moves
	// in constant time - and it bought a second null to check at every use
	// site, which is what `unique_ptr<vector<unique_ptr<T>>>` costs and all it
	// costs.
	std::vector<std::unique_ptr<labrador::CollisionObject>>
		build_collision_objects(const labrador::JsonValue& json,
			const TeamColour& team_colours) const;

	std::vector<std::unique_ptr<labrador::GameObject>>
		build_non_collision_objects(const labrador::JsonValue& json) const;

	std::vector<std::unique_ptr<labrador::GameObject>>
		build_viewport_dividers(const labrador::ViewportManager* viewport_manager) const;

private:
	const PaintTuning* paint_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;
	const labrador::AudioResources* audio_resources_ = nullptr;

	std::unique_ptr<labrador::CollisionObject>
		build_collision_object(const labrador::JsonValue& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<labrador::GameObject>
		build_non_collision_object(const labrador::JsonValue& json) const;
};
