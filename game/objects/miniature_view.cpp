#include "game/pch.h"
#include "game/objects/miniature_view.h"
#include "game/objects/structure.h"

using namespace labrador;
using namespace mattmath;

void draw_miniature_view(DrawList& list, const Camera& camera,
	const Viewport& viewport, std::span<GameObject* const> scenery,
	std::span<CollisionObject* const> collision_objects)
{
	list.set_viewport(viewport);
	list.set_camera(camera);
	const RectangleF visible = camera.visible_rectangle(viewport);
	const float units_per_pixel = 1.0f / camera.scale;
	for (const GameObject* object : scenery)
	{
		if (object->cull_bounds(units_per_pixel).intersects(visible))
		{
			object->draw(list);
		}
	}
	for (const CollisionObject* object : collision_objects)
	{
		if (!object->cull_bounds(units_per_pixel).intersects(visible)) { continue; }
		if (const auto* structure = dynamic_cast<const Structure*>(object))
		{
			structure->draw_miniature(list, camera);
		}
		else
		{
			object->draw(list);
		}
	}
}
