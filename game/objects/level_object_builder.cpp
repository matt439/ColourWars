#include "game/pch.h"
#include "game/objects/level_object_builder.h"
#include "engine/math/triangle.h"
#include <cmath>
#include <initializer_list>

using namespace mattmath;
using namespace labrador;

namespace
{
	// Game assets used to draw the viewport manager's divider rectangles.
	const std::string DIVIDER_SHEET_NAME = "sprite_sheet_1";
	const std::string DIVIDER_FRAME_NAME = "pixel";
	const Colour DIVIDER_COLOUR = Colour::black;

	class ViewportDivider final : public GameObject, public TextureObject
	{
	public:
		ViewportDivider(const ViewportManager* viewport_manager, size_t index,
			RenderResources* render_resources) :
			TextureObject(DIVIDER_SHEET_NAME, DIVIDER_FRAME_NAME,
				render_resources, DIVIDER_COLOUR),
			viewport_manager_(viewport_manager), index_(index)
		{
		}

		void update(float /*dt*/) override {}

		void draw(DrawList& draw_list) const override
		{
			this->TextureObject::draw(draw_list, this->bounds());
		}

		RectangleF bounds() const override
		{
			// The layout stays fixed for a match; its pixel dimensions do not.
			return this->viewport_manager_->viewport_dividers().at(this->index_);
		}

	private:
		const ViewportManager* viewport_manager_;
		size_t index_;
	};

	// Add the source file and object context to an invalid CSS colour error.
	Colour colour_of(const JsonValue& json)
	{
		const std::string name = json.string("colour");
		const std::optional<Colour> colour = colour_from_name(name);
		if (!colour)
		{
			throw std::runtime_error(json.where() +
				": '" + name + "' is not a colour name");
		}
		return *colour;
	}

	// Accept both rectangle encodings; report the object and expected keys
	// if neither is supplied.
	RectangleF checked_rectangle(const JsonValue& json,
		const RectangleF& rectangle)
	{
		if (!std::isfinite(rectangle.x) || !std::isfinite(rectangle.y) ||
			!std::isfinite(rectangle.width) || !std::isfinite(rectangle.height) ||
			rectangle.width <= 0.0f || rectangle.height <= 0.0f)
		{
			throw std::runtime_error(json.where() +
				" needs finite coordinates and positive width and height");
		}
		return rectangle;
	}

	RectangleF decode_rectangle(const JsonValue& json)
	{
		if (json.has("rectangle"))
		{
			const JsonValue rectangle = json.object("rectangle");
			return checked_rectangle(rectangle, RectangleF(
				rectangle.number("x"),
				rectangle.number("y"),
				rectangle.number("width"),
				rectangle.number("height")));
		}
		if (json.has("rectangle_tlbr"))
		{
			const JsonValue rectangle = json.object("rectangle_tlbr");
			return checked_rectangle(rectangle, RectangleF::from_top_left_bottom_right(
				Vector2F(rectangle.number("left"), rectangle.number("top")),
				Vector2F(rectangle.number("right"), rectangle.number("bottom"))));
		}
		throw std::runtime_error(json.where() +
			" has neither a 'rectangle' nor a 'rectangle_tlbr'");
	}

	CollisionObjectType collision_type_named(const JsonValue& json,
		const std::string& name)
	{
		if (name == "STRUCTURE")
		{
			return CollisionObjectType::structure;
		}
		if (name == "STRUCTURE_JUMP_THROUGH")
		{
			return CollisionObjectType::structure_jump_through;
		}
		if (name == "STRUCTURE_PAINTABLE")
		{
			return CollisionObjectType::structure_paintable;
		}
		if (name == "STRUCTURE_RAMP_RIGHT")
		{
			return CollisionObjectType::structure_ramp_right;
		}
		if (name == "STRUCTURE_RAMP_LEFT")
		{
			return CollisionObjectType::structure_ramp_left;
		}
		throw std::runtime_error(json.where() +
			": '" + name + "' is not a collision type");
	}

	// Allowed collision tags for each object type.
	CollisionObjectType collision_type_of(const JsonValue& json,
		const std::string& object_type,
		std::initializer_list<CollisionObjectType> allowed)
	{
		const std::string name = json.string("collision_type");
		const CollisionObjectType type = collision_type_named(json, name);
		for (CollisionObjectType candidate : allowed)
		{
			if (candidate == type)
			{
				return type;
			}
		}
		throw std::runtime_error(json.where() + ": a " + object_type +
			" may not have collision type '" + name + "'");
	}
}

LevelObjectBuilder::LevelObjectBuilder(RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	const PaintTuning* paint) :
	paint_(paint),
	render_resources_(render_resources),
	audio_resources_(audio_resources)
{
	this->preferences_ = preferences;

}

std::unique_ptr<CollisionObject>
	LevelObjectBuilder::build_collision_object(const JsonValue& json,
		const TeamColour& team_colours,
		std::span<const RectangleF> paint_obstacles) const
{
	const std::string type = json.string("type");
	if (type == "Structure")
	{
		const CollisionObjectType collision_type = collision_type_of(json, type,
			{ CollisionObjectType::structure,
			  CollisionObjectType::structure_jump_through });

		const RectangleF rectangle = decode_rectangle(json);

		return std::make_unique<Structure>(
			json.string("sheet_name"),
			json.string("frame_name"),
			rectangle,
			std::make_unique<RectangleF>(rectangle),
			this->render_resources_,
			collision_type,
			colour_of(json));
	}
	if (type == "StructurePaintable")
	{
		const CollisionObjectType collision_type = collision_type_of(json, type,
			{ CollisionObjectType::structure_paintable });

		const RectangleF rectangle = decode_rectangle(json);

		const JsonValue faces_json = json.object("paintable_faces");
		PaintableFaces faces;
		faces.left = faces_json.boolean("left");
		faces.top = faces_json.boolean("top");
		faces.right = faces_json.boolean("right");
		faces.bottom = faces_json.boolean("bottom");

		return std::make_unique<StructurePaintable>(
			json.string("sheet_name"),
			json.string("frame_name"),
			rectangle,
			std::make_unique<RectangleF>(rectangle),
			this->render_resources_,
			this->audio_resources_,
			this->preferences_,
			this->paint_,
			collision_type,
			team_colours,
			faces,
			paint_obstacles,
			colour_of(json));
	}
	if (type == "StructureRamp")
	{
		const CollisionObjectType collision_type = collision_type_of(json, type,
			{ CollisionObjectType::structure_ramp_right,
			  CollisionObjectType::structure_ramp_left });

		const JsonValue triangle_json = json.object("triangle");
		const TriangleRightAxisAligned triangle = TriangleRightAxisAligned(
			triangle_json.number("x1"),
			triangle_json.number("y1"),
			triangle_json.number("x2"),
			triangle_json.number("y2"),
			triangle_json.number("x3"),
			triangle_json.number("y3"));

		// The sheet's ramp frame is right-handed; mirror it for left-handed collision geometry.
		const SpriteFlip flip =
			collision_type == CollisionObjectType::structure_ramp_left
			? SpriteFlip::horizontal
			: SpriteFlip::none;

		return std::make_unique<Structure>(
			json.string("sheet_name"),
			json.string("frame_name"),
			triangle.bounding_box(),
			std::make_unique<TriangleRightAxisAligned>(triangle),
			this->render_resources_,
			collision_type,
			colour_of(json),
			0.0f,
			Vector2F::ZERO,
			flip);
	}

	throw std::runtime_error(json.where() +
		": '" + type + "' is not a collision object type");
}

std::unique_ptr<GameObject>
	LevelObjectBuilder::build_non_collision_object(const JsonValue& json) const
{
	const std::string type = json.string("type");
	if (type == "Visual")
	{
		return std::make_unique<Visual>(
			json.string("sheet_name"),
			json.string("frame_name"),
			decode_rectangle(json),
			this->render_resources_,
			colour_of(json));
	}

	throw std::runtime_error(json.where() +
		": '" + type + "' is not a non-collision object type");
}

std::vector<std::unique_ptr<CollisionObject>>
	LevelObjectBuilder::build_collision_objects(const JsonValue& json,
		const TeamColour& team_colours) const
{
	// Only flat structures block paint. Ramps deliberately let projectiles pass
	// through, and decorative art must not remove reachable scoring surface.
	std::vector<RectangleF> paint_obstacles;
	for (size_t index = 0; index < json.size(); ++index)
	{
		const JsonValue object = json.at(index);
		const std::string type = object.string("type");
		if (type == "Structure" || type == "StructurePaintable")
		{
			paint_obstacles.push_back(decode_rectangle(object));
		}
	}

	std::vector<std::unique_ptr<CollisionObject>> collision_objects;
	collision_objects.reserve(json.size());
	for (size_t index = 0; index < json.size(); ++index)
	{
		collision_objects.push_back(
			build_collision_object(json.at(index), team_colours, paint_obstacles));
	}
	return collision_objects;
}

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_non_collision_objects(const JsonValue& json) const
{
	std::vector<std::unique_ptr<GameObject>> non_collision_objects;
	non_collision_objects.reserve(json.size());
	for (size_t index = 0; index < json.size(); ++index)
	{
		non_collision_objects.push_back(
			build_non_collision_object(json.at(index)));
	}
	return non_collision_objects;
}

void LevelObjectBuilder::validate_objects(const JsonValue& collision_objects,
	const JsonValue& non_collision_objects) const
{
	const TeamColour colours;
	for (size_t index = 0; index < collision_objects.size(); ++index)
	{
		const JsonValue object = collision_objects.at(index);
		try
		{
			build_collision_object(object, colours);
		}
		catch (const std::exception& error)
		{
			throw std::runtime_error(object.where() + ": " + error.what());
		}
	}
	for (size_t index = 0; index < non_collision_objects.size(); ++index)
	{
		const JsonValue object = non_collision_objects.at(index);
		try
		{
			build_non_collision_object(object);
		}
		catch (const std::exception& error)
		{
			throw std::runtime_error(object.where() + ": " + error.what());
		}
	}
}

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_viewport_dividers(
		const ViewportManager* viewport_manager) const
{
	std::vector<std::unique_ptr<GameObject>> viewport_dividers;

	const std::vector<RectangleF> viewport_rectangles =
		viewport_manager->viewport_dividers();
	for (size_t index = 0; index < viewport_rectangles.size(); ++index)
	{
		viewport_dividers.push_back(std::make_unique<ViewportDivider>(
			viewport_manager, index, this->render_resources_));
	}

	return viewport_dividers;
}
