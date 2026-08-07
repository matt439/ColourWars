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

	std::string level_name() const;
	std::vector<mattmath::Vector2F> team_a_spawns() const;
	std::vector<mattmath::Vector2F> team_b_spawns() const;

	mattmath::RectangleF out_of_bounds_rectangle() const;
	mattmath::RectangleF camera_bounds_rectangle() const;

	mattmath::RectangleF zoom_out_start_bounds_rectangle() const;
	mattmath::RectangleF zoom_out_finish_bounds_rectangle() const;

	const rapidjson::Value& collision_objects_json() const;
	const rapidjson::Value& non_collision_objects_json() const;

	std::string sound_bank_name() const;
	std::string music_name() const;
	float music_volume() const;

private:
	rapidjson::Document json_doc_;

	static std::vector<mattmath::Vector2F>
		decode_team_spawns_json(const rapidjson::Value& json);

	static rapidjson::Document load_from_json(const char* json_path);

};
