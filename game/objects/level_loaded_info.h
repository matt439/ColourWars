#pragma once

#include "engine/assets/json.h"
#include "engine/input/gamepads.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
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

	// Reads every key this class exposes, and throws on the first one that is
	// missing or the wrong type, naming the file and the key.
	//
	// WHY THIS EXISTS. Every accessor below is a fresh walk of the document
	// performed on demand, and the manifest callback that loads a level calls
	// exactly two of them - display_name and icon_frame, the two the stage
	// menu needs to draw a row. The other eleven were first read by
	// LevelBuilder inside GameLevel::init(), which is to say after a player
	// picked a stage and pressed A. A level file missing a bounds rectangle
	// therefore killed a session that four people were already sitting in,
	// through a message box captioned "startup failure", at the one moment it
	// could do the most damage. CLAUDE.md's own rule is that a stage the
	// manifest names and the disk cannot supply "must fail at startup, not
	// three screens later"; the disk half was enforced and the contents half
	// was not.
	//
	// Resource names and object entries are checked by validate_content after
	// the complete manifest has loaded, including its optional sound banks.
	void validate() const;
	void validate_content(const LevelObjectBuilder& builder,
		const labrador::RenderResources& render_resources,
		const labrador::AudioResources& audio_resources) const;

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
	labrador::JsonValue collision_objects_json() const;
	labrador::JsonValue non_collision_objects_json() const;

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
	labrador::JsonDocument json_doc_;

	// The four bounds rectangles differ only in which key they sit under.
	mattmath::RectangleF bounds_rectangle(const char* key) const;

	static std::vector<mattmath::Vector2F>
		decode_team_spawns(const labrador::JsonValue& json);

	// Both teams' spawns are checked the same way and want the same sentence.
	static void validate_spawn(const labrador::JsonValue& root,
		const mattmath::Vector2F& spawn,
		const mattmath::RectangleF& out_of_bounds,
		const char* team);
};
