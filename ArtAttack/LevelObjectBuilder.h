#ifndef LEVELOBJECTBUILDER_H
#define LEVELOBJECTBUILDER_H

#include "ICollisionGameObject.h"
#include <memory>
#include "rapidjson/document.h"
#include "Structure.h"
#include "StructurePaintable.h"
#include "Visual.h"
#include "ViewportManager.h"

class LevelObjectBuilder
{
private:
	ResourceManager* _resource_manager = nullptr;
	DirectX::SpriteBatch* _sprite_batch = nullptr;
	const float* _dt = nullptr;

	std::unique_ptr<ICollisionGameObject>
		build_collision_object(const rapidjson::Value& json,
			const team_colour& team_colours) const;

	std::unique_ptr<IGameObject>
		build_non_collision_object(const rapidjson::Value& json) const;

public:
	LevelObjectBuilder(ResourceManager* resource_manager,
		DirectX::SpriteBatch* sprite_batch,
		const float* dt) :
		_resource_manager(resource_manager),
		_sprite_batch(sprite_batch),
		_dt(dt) {}


	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> 
		build_collision_objects(const rapidjson::Value& json,
			const team_colour& team_colours) const;

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> 
		build_non_collision_objects(const rapidjson::Value& json) const;

	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> 
		build_viewport_dividers(const ViewportManager* viewport_manager) const;
};

#endif // !LEVELOBJECTBUILDER_H
