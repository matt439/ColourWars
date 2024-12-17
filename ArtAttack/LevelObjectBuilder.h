#ifndef LEVELOBJECTBUILDER_H
#define LEVELOBJECTBUILDER_H

#include "ICollisionGameObject.h"
#include <memory>
#include "rapidjson/document.h"
#include "StructurePaintable.h"
#include "Visual.h"
#include "ViewportManager.h"

class LevelObjectBuilder
{
public:
	LevelObjectBuilder(ResourceManager* resource_manager,
		const float* dt);

	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> 
		build_collision_objects(const rapidjson::Value& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> 
		build_non_collision_objects(const rapidjson::Value& json) const;

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> 
		build_viewport_dividers(const ViewportManager* viewport_manager) const;

private:
	ResourceManager* _resource_manager = nullptr;
	const float* _dt = nullptr;

	std::unique_ptr<ICollisionGameObject>
		build_collision_object(const rapidjson::Value& json,
			const TeamColour& team_colours) const;

	std::unique_ptr<IGameObject>
		build_non_collision_object(const rapidjson::Value& json) const;
};

#endif // !LEVELOBJECTBUILDER_H
