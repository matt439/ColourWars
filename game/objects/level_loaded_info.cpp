#include "game/pch.h"
#include "game/objects/level_loaded_info.h"
#include "engine/assets/json_loader.h"

using namespace DirectX;
using namespace mattmath;
using namespace rapidjson;
using namespace artattack;

LevelLoadedInfo::LevelLoadedInfo(const char* json_path) :
	json_doc_(load_from_json(json_path))
{

}

std::string LevelLoadedInfo::level_name() const
{
	return this->json_doc_["level_name"].GetString();
}
std::vector<Vector2F> LevelLoadedInfo::team_a_spawns() const
{ 
	const Value& spawn_locations_a = this->json_doc_["spawn_locations"]["team_a"];
	return this->decode_team_spawns_json(spawn_locations_a);
}
std::vector<Vector2F> LevelLoadedInfo::team_b_spawns() const
{ 
	const Value& spawn_locations_b = this->json_doc_["spawn_locations"]["team_b"];
	return this->decode_team_spawns_json(spawn_locations_b);
}

RectangleF LevelLoadedInfo::out_of_bounds_rectangle() const
{
	return RectangleF(
		this->json_doc_["level_bounds"]["out_of_bounds"]["x"].GetFloat(),
		this->json_doc_["level_bounds"]["out_of_bounds"]["y"].GetFloat(),
		this->json_doc_["level_bounds"]["out_of_bounds"]["width"].GetFloat(),
		this->json_doc_["level_bounds"]["out_of_bounds"]["height"].GetFloat());
}
RectangleF LevelLoadedInfo::camera_bounds_rectangle() const
{
	return RectangleF(
		this->json_doc_["level_bounds"]["camera_bounds"]["x"].GetFloat(),
		this->json_doc_["level_bounds"]["camera_bounds"]["y"].GetFloat(),
		this->json_doc_["level_bounds"]["camera_bounds"]["width"].GetFloat(),
		this->json_doc_["level_bounds"]["camera_bounds"]["height"].GetFloat());
}
RectangleF LevelLoadedInfo::zoom_out_start_bounds_rectangle() const
{
	return RectangleF(
		this->json_doc_["level_bounds"]["zoom_out_start_bounds"]["x"].GetFloat(),
		this->json_doc_["level_bounds"]["zoom_out_start_bounds"]["y"].GetFloat(),
		this->json_doc_["level_bounds"]["zoom_out_start_bounds"]["width"].GetFloat(),
		this->json_doc_["level_bounds"]["zoom_out_start_bounds"]["height"].GetFloat());

}
RectangleF LevelLoadedInfo::zoom_out_finish_bounds_rectangle() const
{
	return RectangleF(
		this->json_doc_["level_bounds"]["zoom_out_finish_bounds"]["x"].GetFloat(),
		this->json_doc_["level_bounds"]["zoom_out_finish_bounds"]["y"].GetFloat(),
		this->json_doc_["level_bounds"]["zoom_out_finish_bounds"]["width"].GetFloat(),
		this->json_doc_["level_bounds"]["zoom_out_finish_bounds"]["height"].GetFloat());
}

const Value& LevelLoadedInfo::collision_objects_json() const
{
	return this->json_doc_["collision_objects"];
}
const Value& LevelLoadedInfo::non_collision_objects_json() const
{
	return this->json_doc_["non_collision_objects"];
}
std::string LevelLoadedInfo::sound_bank_name() const
{
	return this->json_doc_["sound"]["sound_bank_name"].GetString();
}
std::string LevelLoadedInfo::music_name() const
{
	return this->json_doc_["sound"]["music_name"].GetString();
}
float LevelLoadedInfo::music_volume() const
{
	return this->json_doc_["sound"]["music_volume"].GetFloat();
}

std::vector<Vector2F>
	LevelLoadedInfo::decode_team_spawns_json(const Value& json)
{
	std::vector<Vector2F> spawns;
	for (auto& spawn : json.GetArray())
	{
		spawns.push_back(Vector2F(spawn["x"].GetFloat(), spawn["y"].GetFloat()));
	}
	return spawns;
}

Document LevelLoadedInfo::load_from_json(const char* json_path)
{
	return read_json_file(json_path);
}