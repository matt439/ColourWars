#pragma once

#include "game/play_settings.h"

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
	// Borrowed paint tuning must outlive the built objects. It sets tile granularity.
	LevelObjectBuilder(labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const PaintTuning* paint);

	std::vector<std::unique_ptr<labrador::CollisionObject>>
		build_collision_objects(const labrador::JsonValue& json,
			const TeamColour& team_colours) const;

	std::vector<std::unique_ptr<labrador::GameObject>>
		build_non_collision_objects(const labrador::JsonValue& json) const;

	std::vector<std::unique_ptr<labrador::GameObject>>
		build_viewport_dividers(const labrador::ViewportManager* viewport_manager) const;

	// Run the actual construction path before a stage becomes playable. Each
	// temporary object is discarded immediately; no scene or sounds are run.
	void validate_objects(const labrador::JsonValue& collision_objects,
		const labrador::JsonValue& non_collision_objects) const;

private:
	const PaintTuning* paint_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;
	const labrador::AudioResources* audio_resources_ = nullptr;
	const PlaySettings* preferences_ = nullptr;

	std::unique_ptr<labrador::CollisionObject>
		build_collision_object(const labrador::JsonValue& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<labrador::GameObject>
		build_non_collision_object(const labrador::JsonValue& json) const;
};
