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

	// How the stage-select menu names and pictures this level. They live in
	// the level file rather than in a table beside the menu, so that adding a
	// stage is a .json plus a manifest line and nothing else (T7).
	std::string display_name() const;
	std::string icon_frame() const;

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
	std::string path_;

	// Reads a string the level file must carry, and throws naming the file and
	// the key if it does not. The rest of this class still reaches into the
	// document unchecked - that is the loaders' standing validation debt, and
	// it is not paid here. These two are checked because they are the keys a
	// person adding a stage has to write by hand, so getting one wrong is the
	// likely mistake rather than a corrupt file (T6).
	std::string required_string(const char* key) const;

	static std::vector<mattmath::Vector2F>
		decode_team_spawns_json(const rapidjson::Value& json);

	static rapidjson::Document load_from_json(const char* json_path);

};
