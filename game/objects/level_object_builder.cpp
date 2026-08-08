#include "game/pch.h"
#include "game/objects/level_object_builder.h"

using namespace rapidjson;
using namespace mattmath;
using namespace artattack;

namespace
{
	// colour_from_name now says so when a name is not one of the CSS colours,
	// instead of quietly answering white. This file already throws on every
	// other unreadable field, so it throws on this one too - and names it,
	// which "invalid colour" would not.
	Colour colour_named(const char* name)
	{
		const std::optional<Colour> colour = colour_from_name(name);
		if (!colour)
		{
			throw std::runtime_error(
				std::string("Unknown colour name: ") + name);
		}
		return *colour;
	}
}

LevelObjectBuilder::LevelObjectBuilder(RenderResources* render_resources,
	const AudioResources* audio_resources) :
	render_resources_(render_resources),
	audio_resources_(audio_resources)
{

}

std::unique_ptr<artattack::CollisionObject>
	LevelObjectBuilder::build_collision_object(const Value& json,
		const TeamColour& team_colours) const
{
	std::string type = json["type"].GetString();
	if (type == "Structure")
	{
		CollisionObjectType col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE")
		{
			col_type = CollisionObjectType::structure;
		}
		else if (collision_type == "STRUCTURE_JUMP_THROUGH")
		{
			col_type = CollisionObjectType::structure_jump_through;
		}
		else
		{
			throw std::exception("Invalid collision type");
		}

		RectangleF rectangle = RectangleF::ZERO;
		if (json.HasMember("rectangle"))
		{
			rectangle = RectangleF(json["rectangle"]["x"].GetFloat(),
				json["rectangle"]["y"].GetFloat(),
				json["rectangle"]["width"].GetFloat(),
				json["rectangle"]["height"].GetFloat());
		}
		else if (json.HasMember("rectangle_tlbr"))
		{
			Vector2F top_left = Vector2F(json["rectangle_tlbr"]["left"].GetFloat(),
				json["rectangle_tlbr"]["top"].GetFloat());
			Vector2F bottom_right = Vector2F(json["rectangle_tlbr"]["right"].GetFloat(),
				json["rectangle_tlbr"]["bottom"].GetFloat());
			rectangle = RectangleF::from_top_left_bottom_right(top_left, bottom_right);
		}
		else
		{
			throw std::exception("Invalid rectangle type");
		}

		return std::make_unique<Structure>(
			json["sheet_name"].GetString(),
			json["frame_name"].GetString(),
			rectangle,
			&rectangle,
			this->render_resources_,
			col_type,
			colour_named(json["colour"].GetString()));
	}
	if (type == "StructurePaintable")
	{
		CollisionObjectType col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE_PAINTABLE")
		{
			col_type = CollisionObjectType::structure_paintable;
		}
		else
		{
			throw std::exception("Invalid collision type");
		}

		RectangleF rectangle = RectangleF::ZERO;
		if (json.HasMember("rectangle"))
		{
			rectangle = RectangleF(json["rectangle"]["x"].GetFloat(),
				json["rectangle"]["y"].GetFloat(),
				json["rectangle"]["width"].GetFloat(),
				json["rectangle"]["height"].GetFloat());
		}
		else if (json.HasMember("rectangle_tlbr"))
		{
			Vector2F top_left = Vector2F(json["rectangle_tlbr"]["left"].GetFloat(),
				json["rectangle_tlbr"]["top"].GetFloat());
			Vector2F bottom_right = Vector2F(json["rectangle_tlbr"]["right"].GetFloat(),
				json["rectangle_tlbr"]["bottom"].GetFloat());
			rectangle = RectangleF::from_top_left_bottom_right(top_left, bottom_right);
		}
		else
		{
			throw std::exception("Invalid rectangle type");
		}

		if (!json.HasMember("paintable_faces"))
		{
			throw std::runtime_error(
				"StructurePaintable is missing its paintable_faces object");
		}

		// Pass the booleans straight through. Encoding them as Segments and
		// recovering them by positional index against edges() put three of
		// the four faces on the wrong side of the structure.
		const rapidjson::Value& faces_json = json["paintable_faces"];
		PaintableFaces faces;
		faces.left = faces_json["left"].GetBool();
		faces.top = faces_json["top"].GetBool();
		faces.right = faces_json["right"].GetBool();
		faces.bottom = faces_json["bottom"].GetBool();

		return std::make_unique<StructurePaintable>(
			json["sheet_name"].GetString(),
			json["frame_name"].GetString(),
			rectangle,
			&rectangle,
			this->render_resources_,
			this->audio_resources_,
			col_type,
			team_colours,
			faces,
			colour_named(json["colour"].GetString()));
	}
	if (type == "StructureRamp")
	{
		CollisionObjectType col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE_RAMP_RIGHT")
		{
			col_type = CollisionObjectType::structure_ramp_right;
		}
		else if (collision_type == "STRUCTURE_RAMP_LEFT")
		{
			col_type = CollisionObjectType::structure_ramp_left;
		}
		else
		{
			throw std::exception("Invalid collision type");
		}
		
		TriangleRightAxisAligned triangle = TriangleRightAxisAligned(
			json["triangle"]["x1"].GetFloat(),
			json["triangle"]["y1"].GetFloat(),
			json["triangle"]["x2"].GetFloat(),
			json["triangle"]["y2"].GetFloat(),
			json["triangle"]["x3"].GetFloat(),
			json["triangle"]["y3"].GetFloat());

		return std::make_unique<Structure>(
			json["sheet_name"].GetString(),
			json["frame_name"].GetString(),
			triangle.bounding_box(),
			&triangle,
			this->render_resources_,
			col_type,
			colour_named(json["colour"].GetString()));
	}

	throw std::exception("Invalid collision object type");
}

std::unique_ptr<GameObject>
	LevelObjectBuilder::build_non_collision_object(const Value& json) const
{
	std::string type = json["type"].GetString();
	if (type == "Visual")
	{
		return std::make_unique<Visual>(
			json["sheet_name"].GetString(),
			json["frame_name"].GetString(),
			RectangleF(json["rectangle"]["x"].GetFloat(),
				json["rectangle"]["y"].GetFloat(),
				json["rectangle"]["width"].GetFloat(),
				json["rectangle"]["height"].GetFloat()),
			this->render_resources_,
			colour_named(json["colour"].GetString()));
	}

	throw std::exception("Invalid non-collision object type");
}

std::vector<std::unique_ptr<artattack::CollisionObject>>
	LevelObjectBuilder::build_collision_objects(const Value& json,
		const TeamColour& team_colours) const
{
	std::vector<std::unique_ptr<artattack::CollisionObject>> collision_objects;
	for (auto& object : json.GetArray())
	{
		collision_objects.push_back(build_collision_object(object, team_colours));
	}
	return collision_objects;
}

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_non_collision_objects(const Value& json) const
{
	std::vector<std::unique_ptr<GameObject>> non_collision_objects;
	for (auto& object : json.GetArray())
	{
		non_collision_objects.push_back(build_non_collision_object(object));
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
			ViewportManager::DIVIDER_SHEET_NAME,
			ViewportManager::DIVIDER_FRAME_NAME,
			rectangle,
			this->render_resources_,
			ViewportManager::DIVIDER_COLOUR));
	}

	return viewport_dividers;
}