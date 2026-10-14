#pragma once

#include "engine/assets/json.h"
#include "engine/input/gamepads.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "game/objects/level_station.h"
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

private:
	// Checked JSON access reports the source file and key on failure.
	labrador::JsonDocument json_doc_;

	// The four bounds rectangles differ only in which key they sit under.
	mattmath::RectangleF bounds_rectangle(const char* key) const;

	static std::vector<mattmath::Vector2F>
		decode_team_spawns(const labrador::JsonValue& json);

	// Spawns and stations are checked the same way and want the same sentence;
	// `what` is "a team_a spawn", "a station 'high_step'".
	static void validate_position(const labrador::JsonValue& root,
		const mattmath::Vector2F& position,
		const mattmath::RectangleF& out_of_bounds,
		const std::string& what);

	void validate_stations(const mattmath::RectangleF& out_of_bounds) const;
};
