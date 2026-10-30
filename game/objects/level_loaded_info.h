#pragma once

#include "engine/assets/json.h"
#include "engine/input/gamepads.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "game/objects/level_station.h"
#include <cstdint>
#include <string>
#include <vector>

class LevelObjectBuilder;
namespace labrador
{
	class RenderResources;
	class AudioResources;
}

class LevelLoadedInfo
{
public:
	explicit LevelLoadedInfo(const char* json_path);

	// The loaded JSON meaning, including authored member/array order. Whitespace,
	// equivalent escapes and equal numeric spellings do not alter it. No disk read.
	// This metadata supports Debug replay checks; layout is the same in Release.
	std::uint64_t semantic_hash() const { return this->semantic_hash_; }

	// Validate required keys and spawn bounds at startup; errors name the file and key.
	// validate_content checks resource names and object entries after the complete
	// manifest has loaded, including optional sound banks.
	void validate() const;
	void validate_content(const LevelObjectBuilder& builder,
		const labrador::RenderResources& render_resources,
		const labrador::AudioResources& audio_resources) const;

	// Display name and legacy icon metadata, authored in the level file.
	// The stage menu renders live arena objects rather than the legacy icon.
	std::string display_name() const;
	std::string icon_frame() const;
	bool include_in_random() const;

	std::vector<mattmath::Vector2F> team_a_spawns() const;
	std::vector<mattmath::Vector2F> team_b_spawns() const;

	// Named places a developer can `goto`: the optional "stations" array
	// ({ "name", "x", "y" }, the character's top-left, like a spawn) in file
	// order, then spawn_a and spawn_b (each team's first spawn), which every
	// level has.
	std::vector<LevelStation> stations() const;
	// The stations' optional "lesson" objects ({ "title", "text", "area" }),
	// in station order: what untimed solo practice teaches (POL#19). Empty for
	// a level without lessons, which therefore has no practice workshop.
	std::vector<LevelLesson> lessons() const;

	mattmath::RectangleF out_of_bounds_rectangle() const;
	mattmath::RectangleF camera_bounds_rectangle() const;

	mattmath::RectangleF zoom_out_start_bounds_rectangle() const;
	mattmath::RectangleF zoom_out_finish_bounds_rectangle() const;

	// Object entries are decoded by LevelObjectBuilder.
	labrador::JsonValue collision_objects_json() const;
	labrador::JsonValue non_collision_objects_json() const;

	std::string sound_bank_name() const;
	// A stage may leave out music_name and music_volume together: it is silent.
	bool has_music() const;
	// Empty and 0 for a silent stage.
	std::string music_name() const;
	float music_volume() const;
	// Optional as a pair, independent of the looping arena music.
	bool has_end_sting() const;
	std::string end_sting_name() const;
	float end_sting_volume() const;

private:
	LevelLoadedInfo(const char* json_path, std::string snapshot);

	// Checked JSON access reports the source file and key on failure.
	labrador::JsonDocument json_doc_;
	std::uint64_t semantic_hash_ = 0;

	// The four bounds rectangles differ only in which key they sit under.
	mattmath::RectangleF bounds_rectangle(const char* bounds_key) const;

	static std::vector<mattmath::Vector2F>
		decode_team_spawns(const labrador::JsonValue& json);

	// Spawns and stations are checked the same way and want the same sentence;
	// `what` is "a team_a spawn", "a station 'high_step'".
	static void validate_position(const labrador::JsonValue& root,
		const mattmath::Vector2F& position,
		const mattmath::RectangleF& out_of_bounds,
		const std::string& what);

	void validate_stations(const mattmath::RectangleF& out_of_bounds) const;
	// A lesson's area lies inside the camera bounds and contains its station;
	// its title and text are non-blank and drawable.
	void validate_lesson(const labrador::JsonValue& lesson,
		const mattmath::Vector2F& station, const std::string& name) const;
	static mattmath::RectangleF lesson_area(const labrador::JsonValue& lesson);
};
