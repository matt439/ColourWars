#ifndef LEVELLOADEDINFO_H
#define LEVELLOADEDINFO_H

#include <cstdio>
#include "rapidjson/document.h"
#include <string>
#include <vector>
#include <map>
#include "MattMath.h"

class LevelLoadedInfo
{
public:
	explicit LevelLoadedInfo(const char* json_path);

	std::string get_level_name() const;
	std::vector<MattMath::Vector2F> get_team_a_spawns() const;
	std::vector<MattMath::Vector2F> get_team_b_spawns() const;

	MattMath::RectangleF get_out_of_bounds_rectangle() const;
	MattMath::RectangleF get_camera_bounds_rectangle() const;

	MattMath::RectangleF get_zoom_out_start_bounds_rectangle() const;
	MattMath::RectangleF get_zoom_out_finish_bounds_rectangle() const;

	const rapidjson::Value& get_collision_objects_json() const;
	const rapidjson::Value& get_non_collision_objects_json() const;

	std::string get_sound_bank_name() const;
	std::string get_music_name() const;
	float get_music_volume() const;

private:
	rapidjson::Document _json_doc;

	static std::vector<MattMath::Vector2F>
		decode_team_spawns_json(const rapidjson::Value& json);

	static rapidjson::Document load_from_json(const char* json_path);

};
#endif // !LEVEL_LOADED_INFO_H
