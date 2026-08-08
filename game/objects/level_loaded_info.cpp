#include "game/pch.h"
#include "game/objects/level_loaded_info.h"

using namespace mattmath;
using namespace artattack;

LevelLoadedInfo::LevelLoadedInfo(const char* json_path) :
	json_doc_(read_json_file(json_path))
{

}

std::string LevelLoadedInfo::display_name() const
{
	return this->json_doc_.root().string("display_name");
}
std::string LevelLoadedInfo::icon_frame() const
{
	return this->json_doc_.root().string("icon_frame");
}
std::vector<Vector2F> LevelLoadedInfo::team_a_spawns() const
{
	return decode_team_spawns(
		this->json_doc_.root().object("spawn_locations").array("team_a"));
}
std::vector<Vector2F> LevelLoadedInfo::team_b_spawns() const
{
	return decode_team_spawns(
		this->json_doc_.root().object("spawn_locations").array("team_b"));
}

RectangleF LevelLoadedInfo::bounds_rectangle(const char* key) const
{
	const JsonValue bounds =
		this->json_doc_.root().object("level_bounds").object(key);

	return RectangleF(
		bounds.number("x"),
		bounds.number("y"),
		bounds.number("width"),
		bounds.number("height"));
}

RectangleF LevelLoadedInfo::out_of_bounds_rectangle() const
{
	return this->bounds_rectangle("out_of_bounds");
}
RectangleF LevelLoadedInfo::camera_bounds_rectangle() const
{
	return this->bounds_rectangle("camera_bounds");
}
RectangleF LevelLoadedInfo::zoom_out_start_bounds_rectangle() const
{
	return this->bounds_rectangle("zoom_out_start_bounds");
}
RectangleF LevelLoadedInfo::zoom_out_finish_bounds_rectangle() const
{
	return this->bounds_rectangle("zoom_out_finish_bounds");
}

JsonValue LevelLoadedInfo::collision_objects_json() const
{
	return this->json_doc_.root().array("collision_objects");
}
JsonValue LevelLoadedInfo::non_collision_objects_json() const
{
	return this->json_doc_.root().array("non_collision_objects");
}
std::string LevelLoadedInfo::sound_bank_name() const
{
	return this->json_doc_.root().object("sound").string("sound_bank_name");
}
std::string LevelLoadedInfo::music_name() const
{
	return this->json_doc_.root().object("sound").string("music_name");
}
float LevelLoadedInfo::music_volume() const
{
	return this->json_doc_.root().object("sound").number("music_volume");
}

std::vector<Vector2F>
	LevelLoadedInfo::decode_team_spawns(const JsonValue& json)
{
	std::vector<Vector2F> spawns;
	spawns.reserve(json.size());
	for (size_t index = 0; index < json.size(); ++index)
	{
		const JsonValue spawn = json.at(index);
		spawns.push_back(Vector2F(spawn.number("x"), spawn.number("y")));
	}
	return spawns;
}
