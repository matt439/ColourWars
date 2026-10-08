#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_overlay_draw.h"
#include "engine/math/shape.h"
#include "engine/render/camera_tools.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/player.h"

using namespace labrador;
using namespace mattmath;

namespace
{
	// Developer-only colours; presentation.json ships, these overlays do not.
	const Colour structure_colour(1.0f, 1.0f, 1.0f, 0.85f);
	const Colour paintable_colour(0.35f, 1.0f, 0.45f, 0.85f);
	const Colour jump_through_colour(0.3f, 0.95f, 1.0f, 0.85f);
	const Colour ramp_colour(1.0f, 0.6f, 0.2f, 0.85f);
	const Colour projectile_colour(1.0f, 0.95f, 0.3f, 0.7f);
	const Colour out_of_bounds_colour(1.0f, 0.25f, 0.25f, 0.9f);
	const Colour scroll_border_colour(1.0f, 0.3f, 1.0f, 0.9f);
	const Colour camera_bounds_colour(0.35f, 0.55f, 1.0f, 0.9f);
	const Colour centre_colour(1.0f, 1.0f, 1.0f, 1.0f);

	// Two window pixels wide at the view's scale.
	constexpr float line_pixels = 2.0f;

	struct Pen
	{
		DrawList& list;
		const SpriteSheet& sheet;
		SpriteSheet::frame_handle fill;
		float thickness;

		void box(const RectangleF& r, const Colour& colour) const
		{
			const float t = this->thickness;
			this->sheet.draw(this->list, this->fill, RectangleF(r.x, r.y, r.width, t), colour);
			this->sheet.draw(this->list, this->fill,
				RectangleF(r.x, r.y + r.height - t, r.width, t), colour);
			this->sheet.draw(this->list, this->fill, RectangleF(r.x, r.y, t, r.height), colour);
			this->sheet.draw(this->list, this->fill,
				RectangleF(r.x + r.width - t, r.y, t, r.height), colour);
		}

		void dot(const Vector2F& centre, float size, const Colour& colour) const
		{
			this->sheet.draw(this->list, this->fill,
				RectangleF(centre.x - size / 2.0f, centre.y - size / 2.0f, size, size), colour);
		}
	};

	Colour colour_for(CollisionObjectType type, const CollisionObject& object, bool& ramp)
	{
		ramp = false;
		switch (type)
		{
		case CollisionObjectType::structure: return structure_colour;
		case CollisionObjectType::structure_paintable: return paintable_colour;
		case CollisionObjectType::structure_jump_through: return jump_through_colour;
		case CollisionObjectType::structure_ramp_left:
		case CollisionObjectType::structure_ramp_right:
			ramp = true;
			return ramp_colour;
		case CollisionObjectType::player_team_a:
		case CollisionObjectType::player_team_b:
		{
			const Player* player = dynamic_cast<const Player*>(&object);
			return player != nullptr ? player->team_colour() : structure_colour;
		}
		default:
			return projectile_colour;
		}
	}
}

void debug_overlay::draw_pane(DrawList& list, const LevelOverlays& overlays,
	const Scene& scene, const Scene::View& view, const Player& player,
	const RectangleF& camera_bounds, const SpriteSheet& sheet,
	SpriteSheet::frame_handle fill)
{
	if (!overlays.any() || view.camera.scale <= 0.0f)
	{
		return;
	}
	list.set_camera(view.camera);
	list.set_filter(TextureFilter::point);
	const Pen pen{ list, sheet, fill, line_pixels / view.camera.scale };
	const Viewport pixels(view.viewport.pixel_rect());
	const RectangleF visible = view.camera.visible_rectangle(pixels);

	if (overlays.collision)
	{
		for (const CollisionObject* object : scene.collision_objects())
		{
			// The narrow-phase shape, not the broad-phase bounds (a player's
			// is padded for its weapon).
			const RectangleF bounds = object->shape()->bounding_box();
			if (!bounds.intersects(visible))
			{
				continue;
			}
			bool ramp = false;
			const CollisionObjectType type = to_collision_type(object->tag());
			const Colour colour = colour_for(type, *object, ramp);
			pen.box(bounds, colour);
			if (ramp)
			{
				// The slope as eight stepped squares: rising to the right for a
				// right ramp, to the left for a left one.
				const bool rises_right = type == CollisionObjectType::structure_ramp_right;
				for (int i = 0; i <= 8; ++i)
				{
					const float along = static_cast<float>(i) / 8.0f;
					const float x = bounds.x + bounds.width * along;
					const float y = rises_right ? bounds.y + bounds.height * (1.0f - along) :
						bounds.y + bounds.height * along;
					pen.dot(Vector2F(x, y), pen.thickness * 3.0f, colour);
				}
			}
		}
		if (scene.bounds())
		{
			pen.box(*scene.bounds(), out_of_bounds_colour);
		}
	}

	if (overlays.camera)
	{
		// The dead zone PlayerCamera::follow keeps the player inside, from the
		// same world-unit size it passes to CameraTools.
		const Vector2F world = visible.size();
		const BorderThickness border = CameraTools::calculate_camera_scroll_border(world);
		const Vector2F origin = view.camera.translation;
		pen.box(RectangleF(origin.x + border.left, origin.y + border.top,
			world.x - border.left - border.right, world.y - border.top - border.bottom),
			scroll_border_colour);
		pen.box(camera_bounds, camera_bounds_colour);
		const Vector2F centre = player.center();
		const float arm = 9.0f;
		const float t = pen.thickness;
		sheet.draw(list, fill, RectangleF(centre.x - arm, centre.y - t / 2.0f, arm * 2.0f, t),
			centre_colour);
		sheet.draw(list, fill, RectangleF(centre.x - t / 2.0f, centre.y - arm, t, arm * 2.0f),
			centre_colour);
	}
}
#endif
