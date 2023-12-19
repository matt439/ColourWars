#include "pch.h"
#include "LevelObjectBuilder.h"

using namespace rapidjson;
using namespace MattMath;

std::unique_ptr<ICollisionGameObject>
	LevelObjectBuilder::build_collision_object(const Value& json,
		const team_colour& team_colours) const
{
	std::string type = json["type"].GetString();
	if (type == "Structure")
	{
		collision_object_type col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE_WALL")
		{
			col_type = collision_object_type::STRUCTURE_WALL;
		}
		else if (collision_type == "STRUCTURE_FLOOR")
		{
			col_type = collision_object_type::STRUCTURE_FLOOR;
		}
		else if (collision_type == "STRUCTURE")
		{
			col_type = collision_object_type::STRUCTURE;
		}
		else
		{
			throw std::exception("Invalid collision type");
		}

		return std::make_unique<Structure>(
			json["sheet_name"].GetString(),
			json["frame_name"].GetString(),
				RectangleF(json["rectangle"]["x"].GetFloat(),
					json["rectangle"]["y"].GetFloat(),
					json["rectangle"]["width"].GetFloat(),
					json["rectangle"]["height"].GetFloat()),
				this->_sprite_batch,
				this->_resource_manager,
				col_type,
				colour_consts::colour_from_name(json["colour"].GetString()));
	}
	else if (type == "StructurePaintable")
	{
		collision_object_type col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE_WALL_PAINTABLE")
		{
			col_type = collision_object_type::STRUCTURE_WALL_PAINTABLE;
		}
		else if (collision_type == "STRUCTURE_FLOOR_PAINTABLE")
		{
			col_type = collision_object_type::STRUCTURE_FLOOR_PAINTABLE;
		}
		else
		{
			throw std::exception("Invalid collision type");
		}

		paintable_faces faces;
		faces.left = json["paintable_faces"]["left"].GetBool();
		faces.top = json["paintable_faces"]["top"].GetBool();
		faces.right = json["paintable_faces"]["right"].GetBool();
		faces.bottom = json["paintable_faces"]["bottom"].GetBool();

		return std::make_unique<StructurePaintable>(
			json["sheet_name"].GetString(),
			json["frame_name"].GetString(),
			RectangleF(json["rectangle"]["x"].GetFloat(),
								json["rectangle"]["y"].GetFloat(),
								json["rectangle"]["width"].GetFloat(),
								json["rectangle"]["height"].GetFloat()),
			this->_sprite_batch,
			this->_resource_manager,
			col_type,
			team_colours,
			faces,
			this->_dt,
			colour_consts::colour_from_name(json["colour"].GetString()));
	}

	throw std::exception("Invalid collision object type");
}

std::unique_ptr<IGameObject>
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
			this->_sprite_batch,
			this->_resource_manager,
			colour_consts::colour_from_name(json["colour"].GetString()));
	}

	throw std::exception("Invalid non-collision object type");
}

std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>>
	LevelObjectBuilder::build_collision_objects(const Value& json,
		const team_colour& team_colours) const
{
	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>>
		collision_objects = std::make_unique<std::vector<std::unique_ptr<ICollisionGameObject>>>();
	for (auto& object : json.GetArray())
	{
		collision_objects->push_back(build_collision_object(object, team_colours));
	}
	return collision_objects;

}

std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
	LevelObjectBuilder::build_non_collision_objects(const Value& json) const
{
	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
		non_collision_objects = std::make_unique<std::vector<std::unique_ptr<IGameObject>>>();
	for (auto& object : json.GetArray())
	{
		non_collision_objects->push_back(build_non_collision_object(object));
	}
	return non_collision_objects;
}

std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
	LevelObjectBuilder::build_viewport_dividers(
		const ViewportManager* viewport_manager) const
{
	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
		viewport_dividers = std::make_unique<std::vector<std::unique_ptr<IGameObject>>>();

	std::vector<RectangleF> viewport_rectangles =
		viewport_manager->get_viewport_dividers();
	for (auto& rectangle : viewport_rectangles)
	{
		viewport_dividers->push_back(std::make_unique<Visual>(
			viewport_consts::DIVIDER_SHEET_NAME,
			viewport_consts::DIVIDER_FRAME_NAME,
			rectangle,
			this->_sprite_batch,
			this->_resource_manager,
			viewport_consts::DIVIDER_COLOUR));
	}

	return viewport_dividers;
}