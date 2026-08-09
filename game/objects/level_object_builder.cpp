#include "game/pch.h"
#include "game/objects/level_object_builder.h"
#include <initializer_list>

using namespace mattmath;
using namespace artattack;

namespace
{
	// What a split-screen divider is drawn with. These were constants on the
	// engine's ViewportManager, which named this game's sprite sheet and this
	// game's frame; the engine hands out divider rectangles and this file is
	// the only thing that has ever filled them.
	const std::string DIVIDER_SHEET_NAME = "sprite_sheet_1";
	const std::string DIVIDER_FRAME_NAME = "pixel";
	const Colour DIVIDER_COLOUR = Colour::black;

	// colour_from_name now says so when a name is not one of the CSS colours,
	// instead of quietly answering white. This file already throws on every
	// other unreadable field, so it throws on this one too - and JsonValue
	// says which file and which object, which "invalid colour" would not.
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

	// A level file may give a rectangle either way round, and both spellings
	// are in the shipped levels. Neither is optional in the sense of having a
	// default: an object with no rectangle at all is a level that cannot be
	// built, so say which object and which spellings were looked for.
	RectangleF decode_rectangle(const JsonValue& json)
	{
		if (json.has("rectangle"))
		{
			const JsonValue rectangle = json.object("rectangle");
			return RectangleF(
				rectangle.number("x"),
				rectangle.number("y"),
				rectangle.number("width"),
				rectangle.number("height"));
		}
		if (json.has("rectangle_tlbr"))
		{
			const JsonValue rectangle = json.object("rectangle_tlbr");
			return RectangleF::from_top_left_bottom_right(
				Vector2F(rectangle.number("left"), rectangle.number("top")),
				Vector2F(rectangle.number("right"), rectangle.number("bottom")));
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

	// Which collision types a given object type is allowed to claim. The check
	// used to be a per-branch if-chain repeating the spellings, and its throw
	// said "Invalid collision type" without saying which file, which object or
	// which type.
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
	const AudioResources* audio_resources) :
	render_resources_(render_resources),
	audio_resources_(audio_resources)
{

}

std::unique_ptr<CollisionObject>
	LevelObjectBuilder::build_collision_object(const JsonValue& json,
		const TeamColour& team_colours) const
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

		// Pass the booleans straight through. Encoding them as Segments and
		// recovering them by positional index against edges() put three of
		// the four faces on the wrong side of the structure.
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
			collision_type,
			team_colours,
			faces,
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

		return std::make_unique<Structure>(
			json.string("sheet_name"),
			json.string("frame_name"),
			triangle.bounding_box(),
			std::make_unique<TriangleRightAxisAligned>(triangle),
			this->render_resources_,
			collision_type,
			colour_of(json));
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
		const JsonValue rectangle = json.object("rectangle");

		return std::make_unique<Visual>(
			json.string("sheet_name"),
			json.string("frame_name"),
			RectangleF(rectangle.number("x"),
				rectangle.number("y"),
				rectangle.number("width"),
				rectangle.number("height")),
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
	std::vector<std::unique_ptr<CollisionObject>> collision_objects;
	collision_objects.reserve(json.size());
	for (size_t index = 0; index < json.size(); ++index)
	{
		collision_objects.push_back(
			build_collision_object(json.at(index), team_colours));
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

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_viewport_dividers(
		const ViewportManager* viewport_manager) const
{
	std::vector<std::unique_ptr<GameObject>> viewport_dividers;

	std::vector<RectangleF> viewport_rectangles =
		viewport_manager->viewport_dividers();
	for (auto& rectangle : viewport_rectangles)
	{
		viewport_dividers.push_back(std::make_unique<Visual>(
			DIVIDER_SHEET_NAME,
			DIVIDER_FRAME_NAME,
			rectangle,
			this->render_resources_,
			DIVIDER_COLOUR));
	}

	return viewport_dividers;
}
