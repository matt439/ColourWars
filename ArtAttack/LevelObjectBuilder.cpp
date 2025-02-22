#include "pch.h"
#include "LevelObjectBuilder.h"

using namespace rapidjson;
using namespace MattMath;

LevelObjectBuilder::LevelObjectBuilder(ResourceManager* resource_manager,
	const float* dt) :
	_resource_manager(resource_manager),
	_dt(dt)
{

}

std::unique_ptr<ICollisionGameObject>
	LevelObjectBuilder::build_collision_object(const Value& json,
		const TeamColour& team_colours) const
{
	std::string type = json["type"].GetString();
	if (type == "Structure")
	{
		collision_object_type col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE")
		{
			col_type = collision_object_type::STRUCTURE;
		}
		else if (collision_type == "STRUCTURE_JUMP_THROUGH")
		{
			col_type = collision_object_type::STRUCTURE_JUMP_THROUGH;
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
			this->_resource_manager,
			col_type,
			colour_consts::colour_from_name(json["colour"].GetString()));
	}
	if (type == "StructurePaintable")
	{
		collision_object_type col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE_PAINTABLE")
		{
			col_type = collision_object_type::STRUCTURE_PAINTABLE;
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

		//PaintableFaces faces;
		//faces.left = json["paintable_faces"]["left"].GetBool();
		//faces.top = json["paintable_faces"]["top"].GetBool();
		//faces.right = json["paintable_faces"]["right"].GetBool();
		//faces.bottom = json["paintable_faces"]["bottom"].GetBool();

		std::vector<Segment> paintable_edges;
		if (json.HasMember("paintable_faces"))
		{
			if (json["paintable_faces"]["left"].GetBool())
			{
				paintable_edges.push_back(rectangle.get_left_edge());
			}
			if (json["paintable_faces"]["top"].GetBool())
			{
				paintable_edges.push_back(rectangle.get_top_edge());
			}
			if (json["paintable_faces"]["right"].GetBool())
			{
				paintable_edges.push_back(rectangle.get_right_edge());
			}
			if (json["paintable_faces"]["bottom"].GetBool())
			{
				paintable_edges.push_back(rectangle.get_bottom_edge());
			}
		}
		else
		{
			throw std::exception("Invalid paintable edges");
		}

		return std::make_unique<StructurePaintable>(
			json["sheet_name"].GetString(),
			json["frame_name"].GetString(),
			rectangle,
			&rectangle,
			this->_resource_manager,
			col_type,
			team_colours,
			paintable_edges,
			this->_dt,
			colour_consts::colour_from_name(json["colour"].GetString()));
	}
	if (type == "StructureRamp")
	{
		collision_object_type col_type;
		std::string collision_type = json["collision_type"].GetString();
		if (collision_type == "STRUCTURE_RAMP_RIGHT")
		{
			col_type = collision_object_type::STRUCTURE_RAMP_RIGHT;
		}
		else if (collision_type == "STRUCTURE_RAMP_LEFT")
		{
			col_type = collision_object_type::STRUCTURE_RAMP_LEFT;
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
			triangle.get_bounding_box(),
			&triangle,
			this->_resource_manager,
			col_type,
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
			this->_resource_manager,
			colour_consts::colour_from_name(json["colour"].GetString()));
	}

	throw std::exception("Invalid non-collision object type");
}

std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>>
	LevelObjectBuilder::build_collision_objects(const Value& json,
		const TeamColour& team_colours) const
{
	auto collision_objects = std::make_unique<std::vector<std::unique_ptr<ICollisionGameObject>>>();
	for (auto& object : json.GetArray())
	{
		collision_objects->push_back(build_collision_object(object, team_colours));
	}
	return collision_objects;

}

std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
	LevelObjectBuilder::build_non_collision_objects(const Value& json) const
{
	auto non_collision_objects = std::make_unique<std::vector<std::unique_ptr<IGameObject>>>();
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
	auto viewport_dividers = std::make_unique<std::vector<std::unique_ptr<IGameObject>>>();

	std::vector<RectangleF> viewport_rectangles =
		viewport_manager->get_viewport_dividers();
	for (auto& rectangle : viewport_rectangles)
	{
		viewport_dividers->push_back(std::make_unique<Visual>(
			viewport_consts::DIVIDER_SHEET_NAME,
			viewport_consts::DIVIDER_FRAME_NAME,
			rectangle,
			this->_resource_manager,
			viewport_consts::DIVIDER_COLOUR));
	}

	return viewport_dividers;
}