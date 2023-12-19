#include "pch.h"
#include "LevelLoadedInfo.h"
#include "rapidjson/filereadstream.h"

using namespace DirectX;
using namespace MattMath;
using namespace rapidjson;

std::string LevelLoadedInfo::get_level_name() const
{
	//return this->_level_name;
	return this->_json_doc["level_name"].GetString();
}
std::vector<Vector2F> LevelLoadedInfo::get_team_a_spawns() const
{ 
	const Value& spawn_locations_a = this->_json_doc["spawn_locations"]["team_a"];
	return this->decode_team_spawns_json(spawn_locations_a);
	//return this->decode_team_spawns_json(this->_json_doc["spawn_locations"]["team_a"]);

}
std::vector<Vector2F> LevelLoadedInfo::get_team_b_spawns() const
{ 
	//return this->_team_b_spawns;
	const Value& spawn_locations_b = this->_json_doc["spawn_locations"]["team_b"];
	return this->decode_team_spawns_json(spawn_locations_b);
}
//const std::vector<std::string>& LevelLoadedInfo::get_tile_types() const
//{ 
//	//return this->_tile_types; 
//}
//border_thickness LevelLoadedInfo::get_camera_bounds_border() const
//{
//	const Value& camera_bounds = this->_json_doc["camera_boundary_border"];
//	return this->decode_camera_bounds_json(camera_bounds);
//}

RectangleF LevelLoadedInfo::get_out_of_bounds_rectangle() const
{
	return RectangleF(
		this->_json_doc["level_bounds"]["out_of_bounds"]["x"].GetFloat(),
		this->_json_doc["level_bounds"]["out_of_bounds"]["y"].GetFloat(),
		this->_json_doc["level_bounds"]["out_of_bounds"]["width"].GetFloat(),
		this->_json_doc["level_bounds"]["out_of_bounds"]["height"].GetFloat());
}
RectangleF LevelLoadedInfo::get_camera_bounds_rectangle() const
{
	return RectangleF(
		this->_json_doc["level_bounds"]["camera_bounds"]["x"].GetFloat(),
		this->_json_doc["level_bounds"]["camera_bounds"]["y"].GetFloat(),
		this->_json_doc["level_bounds"]["camera_bounds"]["width"].GetFloat(),
		this->_json_doc["level_bounds"]["camera_bounds"]["height"].GetFloat());
}
RectangleF LevelLoadedInfo::get_zoom_out_start_bounds_rectangle() const
{
	return RectangleF(
		this->_json_doc["level_bounds"]["zoom_out_start_bounds"]["x"].GetFloat(),
		this->_json_doc["level_bounds"]["zoom_out_start_bounds"]["y"].GetFloat(),
		this->_json_doc["level_bounds"]["zoom_out_start_bounds"]["width"].GetFloat(),
		this->_json_doc["level_bounds"]["zoom_out_start_bounds"]["height"].GetFloat());

}
RectangleF LevelLoadedInfo::get_zoom_out_finish_bounds_rectangle() const
{
	return RectangleF(
		this->_json_doc["level_bounds"]["zoom_out_finish_bounds"]["x"].GetFloat(),
		this->_json_doc["level_bounds"]["zoom_out_finish_bounds"]["y"].GetFloat(),
		this->_json_doc["level_bounds"]["zoom_out_finish_bounds"]["width"].GetFloat(),
		this->_json_doc["level_bounds"]["zoom_out_finish_bounds"]["height"].GetFloat());
}

const Value& LevelLoadedInfo::get_collision_objects_json() const
{
	return this->_json_doc["collision_objects"];
}
const Value& LevelLoadedInfo::get_non_collision_objects_json() const
{
	return this->_json_doc["non_collision_objects"];
}
std::string LevelLoadedInfo::get_sound_bank_name() const
{
	return this->_json_doc["sound"]["sound_bank_name"].GetString();
}
std::string LevelLoadedInfo::get_music_name() const
{
	return this->_json_doc["sound"]["music_name"].GetString();
}
float LevelLoadedInfo::get_music_volume() const
{
	return this->_json_doc["sound"]["music_volume"].GetFloat();
}

//std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>>
//	LevelLoadedInfo::get_collision_objects() const
//{
//	const Value& objects = this->_json_doc["collision_objects"];
//	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> collision_objects =
//		std::make_unique<std::vector<std::unique_ptr<ICollisionGameObject>>>();
//
//	for (auto& object : objects.GetArray())
//	{
//		std::string type = object["type"].GetString();
//		if (type == "Structure")
//		{
//			collision_object_type col_type;
//			std::string collision_type = object["collision_type"].GetString();
//			if (collision_type == "structure_wall")
//			{
//				col_type = collision_object_type::STRUCTURE_WALL;
//			}
//			else if (collision_type == "structure_floor")
//			{
//				col_type = collision_object_type::STRUCTURE_FLOOR;
//			}
//			else
//			{
//				throw std::exception("Invalid collision type");
//			}
//			
//			std::unique_ptr<ICollisionGameObject> structure =
//				std::make_unique<Structure>(
//					object["sheet_name"].GetString(),
//					object["frame_name"].GetString(),
//					RectangleF(object["x"].GetFloat(),
//						object["y"].GetFloat(),
//						object["width"].GetFloat(),
//						object["height"].GetFloat()),
//					col_type);
//			collision_objects->push_back(std::move(structure));
//		}
//	}
//
//	return collision_objects;
//}
//std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
//	LevelLoadedInfo::get_non_collision_objects() const
//{
//
//}

//std::vector<std::string> LevelLoadedInfo::decode_tile_types_json(
//	const Value& json) const
//{
//	std::vector<std::string> tile_types;
//	for (auto& tile_type : json.GetArray())
//	{
//		tile_types.push_back(tile_type.GetString());
//	}
//	return tile_types;
//}
std::vector<Vector2F>
	LevelLoadedInfo::decode_team_spawns_json(const Value& json)	const
{
	std::vector<Vector2F> spawns;
	for (auto& spawn : json.GetArray())
	{
		spawns.push_back(Vector2F(spawn["x"].GetFloat(), spawn["y"].GetFloat()));
	}
	return spawns;
}
//border_thickness
//	LevelLoadedInfo::decode_camera_bounds_json(const Value& json) const
//{
//	border_thickness camera_bounds;
//	camera_bounds.left = json["left"].GetFloat();
//	camera_bounds.top = json["top"].GetFloat();
//	camera_bounds.right = json["right"].GetFloat();
//	camera_bounds.bottom = json["bottom"].GetFloat();
//	return camera_bounds;
//}
Document LevelLoadedInfo::load_from_json(const char* json_path) const
{
	FILE* fp = fopen(json_path, "rb");

	std::unique_ptr<char> readBuffer = std::make_unique<char>();
	FileReadStream is(fp, readBuffer.get(), sizeof(readBuffer));

	Document d;
	d.ParseStream(is);

	fclose(fp);
	readBuffer.release();

	return d;

	//this->_json_doc = std::move(d);

	//Value& tile_types = d["tile_types"];
	//Value& spawn_locations_a = d["spawn_locations"]["team_a"];
	//Value& spawn_locations_b = d["spawn_locations"]["team_b"];
	//Value& camera_bounds = d["camera_boundary_border"];
	//
	//this->_level_name = d["level_name"].GetString();
	//this->_tile_types = this->decode_tile_types_json(tile_types);
	//this->_team_a_spawns = this->decode_team_spawns_json(
	//	spawn_locations_a);
	//this->_team_b_spawns = this->decode_team_spawns_json(
	//	spawn_locations_b);
	//this->_camera_bounds_border = this->decode_camera_bounds_json(camera_bounds);
}