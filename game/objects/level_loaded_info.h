#pragma once

#include <cstdio>
#include "rapidjson/document.h"
#include <string>
#include <vector>
#include <map>
#include "engine/math/matt_math.h"

class LevelLoadedInfo
{
public:
	explicit LevelLoadedInfo(const char* json_path);

	std::string get_level_name() const;
	std::vector<mattmath::Vector2F> get_team_a_spawns() const;
	std::vector<mattmath::Vector2F> get_team_b_spawns() const;

	mattmath::RectangleF get_out_of_bounds_rectangle() const;
	mattmath::RectangleF get_camera_bounds_rectangle() const;

	mattmath::RectangleF get_zoom_out_start_bounds_rectangle() const;
	mattmath::RectangleF get_zoom_out_finish_bounds_rectangle() const;

	const rapidjson::Value& get_collision_objects_json() const;
	const rapidjson::Value& get_non_collision_objects_json() const;

	std::string get_sound_bank_name() const;
	std::string get_music_name() const;
	float get_music_volume() const;

private:
	rapidjson::Document json_doc_;

	static std::vector<mattmath::Vector2F>
		decode_team_spawns_json(const rapidjson::Value& json);

	static rapidjson::Document load_from_json(const char* json_path);

};
