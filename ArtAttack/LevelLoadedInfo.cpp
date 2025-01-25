#include "pch.h"
#include "LevelLoadedInfo.h"
#include "rapidjson/filereadstream.h"

using namespace DirectX;
using namespace MattMath;
using namespace rapidjson;

LevelLoadedInfo::LevelLoadedInfo(const char* json_path) :
	_json_doc(load_from_json(json_path))
{

}

std::string LevelLoadedInfo::get_level_name() const
{
	return this->_json_doc["level_name"].GetString();
}
std::vector<Vector2F> LevelLoadedInfo::get_team_a_spawns() const
{ 
	const Value& spawn_locations_a = this->_json_doc["spawn_locations"]["team_a"];
	return this->decode_team_spawns_json(spawn_locations_a);
}
std::vector<Vector2F> LevelLoadedInfo::get_team_b_spawns() const
{ 
	const Value& spawn_locations_b = this->_json_doc["spawn_locations"]["team_b"];
	return this->decode_team_spawns_json(spawn_locations_b);
}

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
	FILE* fp = fopen(json_path, "rb");

	std::unique_ptr<char> readBuffer = std::make_unique<char>();
	FileReadStream is(fp, readBuffer.get(), sizeof(readBuffer));

	Document d;
	d.ParseStream(is);

	fclose(fp);
	readBuffer.release();

	return d;
}