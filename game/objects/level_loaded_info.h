#pragma once

#include "engine/assets/json.h"
#include "engine/math/matt_math.h"
#include <string>
#include <vector>

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

	// The two object lists, handed on undecoded: what an entry in them means is
	// LevelObjectBuilder's vocabulary, not this class's.
	artattack::JsonValue collision_objects_json() const;
	artattack::JsonValue non_collision_objects_json() const;

	std::string sound_bank_name() const;
	std::string music_name() const;
	float music_volume() const;

private:
	// A level file is written by hand, so every read of one is checked and says
	// which file and which key when it fails (T6). Two of the reads below used
	// to be; the rest reached into the document with rapidjson's asserting
	// accessors, which NDEBUG turns from a stopped debug build into a null
	// dereference in a shipped one. JsonValue is what closed that, and it
	// closed it for the engine's own loaders in the same pass.
	artattack::JsonDocument json_doc_;

	// The four bounds rectangles differ only in which key they sit under.
	mattmath::RectangleF bounds_rectangle(const char* key) const;

	static std::vector<mattmath::Vector2F>
		decode_team_spawns(const artattack::JsonValue& json);
};
