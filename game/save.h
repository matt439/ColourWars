#pragma once

#include <iostream>
#include <chrono>
#include "game/directory_consts.h"
#include "engine/math/vector2i.h"
#include "engine/render/screen_resolution.h"
#include "game/objects/team_colour.h"
#include "game/play_settings.h"
#include "game/match_records.h"
#include "game/last_match_setup.h"
#include "game/input_bindings.h"

struct SaveData
{
	// Default to the menus' 1920x1080 design resolution.
	labrador::ScreenResolution resolution = labrador::ScreenResolution::s_1920_1080;
	bool fullscreen = false;
	TeamColourPair team_palette = TeamColourPair::random;
	PlaySettings play;
	InputBindings input_bindings;
	MatchRecords records;
	std::optional<LastMatchSetup> last_match;
};

// The per-user save file. Its content is a list of SaveSection objects read
// from and written to one root object; "save_version" names the layout.
class Save
{
public:
	// 0 is the original flat file with no version key. 1 adds the version key
	// and nested sections ("records") beside the same flat preference keys.
	// 2 adds the independent nested "last_match" setup; older saves omit it.
	// 3 adds independent keyboard Jump preferences under "input_bindings".
	static constexpr int version = 3;

	Save() = default;
	void load_save_file();
	// A failed write leaves the active preferences intact and returns false.
	// Interactive callers report that outcome in their own settings screen.
	bool save_to_file() const;
	SaveData save_data() const;
	bool set_display_settings_and_save(labrador::ScreenResolution resolution,
		bool fullscreen, TeamColourPair palette);
	labrador::ScreenResolution resolution() const;
	bool fullscreen() const;
	TeamColourPair team_palette() const;
	const PlaySettings& play_settings() const;
	bool set_play_settings_and_save(const PlaySettings& settings);
	const InputBindings& input_bindings() const;
	// Like play preferences, a valid choice becomes active before the write;
	// failure keeps it active and leaves the previous disk file intact.
	bool set_input_bindings_and_save(const InputBindings& bindings);
	const MatchRecords& records() const;
	// Counts one finished match on `stage` and writes the save. A failed
	// write keeps the record for this run and reports it on stderr only.
	RecordOutcome record_result_and_save(const std::string& stage,
		const LevelEndInfo& result);
	const std::optional<LastMatchSetup>& last_match_setup() const;
	// Writes a validated resolved setup. Unlike active display/play settings,
	// the remembered setup changes in memory only after a successful write.
	bool remember_match_setup_and_save(const LastMatchSetup& setup);
private:
	SaveData save_data_ = SaveData();
	SaveData load_from_json(const char* json_path) const;
	bool write_save_file(const SaveData& data) const;

	static bool file_exists(const std::string& path);
};
