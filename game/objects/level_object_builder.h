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

	// The airbrush speckle under every exposed underside of a flat structure
	// that wears a material. Scene draws plain objects before collision
	// objects, so these land under all terrain and paint.
	std::vector<std::unique_ptr<labrador::GameObject>>
		build_shadows(const labrador::JsonValue& collision_objects) const;

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

	// What every object in a stage needs from the others: the flat rectangles
	// that bury paint, everything that buries an outline (ramps too: a ramp's
	// leg against a face makes one surface, though paint passes through it),
	// and the x of the stage's mirror line.
	struct StageContext
	{
		std::vector<mattmath::RectangleF> flat;
		std::vector<mattmath::RectangleF> solid;
		float mirror_x = 0.0f;
	};
	static StageContext stage_context(const labrador::JsonValue& collision_objects);

	std::unique_ptr<labrador::CollisionObject>
		build_collision_object(const labrador::JsonValue& json,
			const TeamColour& team_colours,
			const StageContext& context) const;

	std::unique_ptr<labrador::GameObject>
		build_non_collision_object(const labrador::JsonValue& json) const;
};
