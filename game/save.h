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
#include "game/player_profiles.h"
#include <array>

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
	PlayerProfiles profiles;
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
	// Its optional "pads" array adds per-slot Jump/Shoot/Pause controls; absent
	// entries keep the defaults. An older "pad_jump" face-button array is still
	// read when "pads" is absent, without changing version-3 meanings.
	// 4 adds the independent "profiles" list; older saves have none.
	// 5 adds "last_match.stage_rule", the series stage rule; older saves have
	// none and read as Same, which is how they played.
	static constexpr int version = 5;

	Save() = default;
	// Reads the file, or a legacy one, or keeps the defaults. A SettingsSession
	// borrowing this Save rereads through its own load_save_file instead.
	void load_save_file();
	// A failed write leaves the active preferences intact and returns false.
	// Interactive callers report that outcome in their own settings screen.
	bool save_to_file() const;
	// What the file holds: each pad slot's own settings, whatever it wears.
	SaveData save_data() const;
	bool set_display_settings_and_save(labrador::ScreenResolution resolution,
		bool fullscreen, TeamColourPair palette);
	labrador::ScreenResolution resolution() const;
	bool fullscreen() const;
	TeamColourPair team_palette() const;

	// What the file holds: each pad slot's own play settings and buttons,
	// whatever it wears. The settings in effect, with worn profiles laid over
	// them, are SettingsSession's; read those unless the file is the point.
	const PlaySettings& saved_play_settings() const;
	const InputBindings& saved_input_bindings() const;
	// Throws invalid_argument outside the supported ranges; otherwise returns
	// `settings` with each stick deadzone normalized.
	static PlaySettings checked_play_settings(const PlaySettings& settings);
	// The setters without "_and_save" change memory only and are for
	// SettingsSession, which writes once it has made every change.
	// Throws as checked_play_settings and stores the checked value.
	void set_play_settings(const PlaySettings& settings);
	// Throws as InputBindings::validate.
	void set_input_bindings(const InputBindings& bindings);

	// Player profiles (ROAD#21). Which pad wears which profile is the run's,
	// and SettingsSession's; the Save holds only the list.
	const PlayerProfiles& profiles() const;
	// Normalizes the name. Throws invalid_argument for a bad or taken name,
	// or when full, before changing anything. Returns the new index; the
	// write's outcome goes to `saved`, and the profile stays either way.
	int add_profile_and_save(PlayerProfile profile, bool& saved);
	// Throws invalid_argument for an unknown index or a bad or taken name.
	bool rename_profile_and_save(int profile, const std::string& name);
	// Throws invalid_argument outside handicaps::valid or for an unknown index.
	bool set_profile_boost_and_save(int profile, int boost);
	void set_profile_boost(int profile, int boost);
	// A profile's stick, rumble and comfort settings, and its buttons. Throw
	// invalid_argument for an unknown index.
	void set_profile_controls(int profile, const StickSettings& sticks, bool rumble,
		const SeatComfort& comfort);
	void set_profile_buttons(int profile, const PadProfile& buttons);
	// Later profiles' indices shift down. Throws for an unknown index. While
	// a SettingsSession borrows this Save, delete through it so the pads
	// wearing profiles follow the shift.
	bool delete_profile_and_save(int profile);
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
	PlayerProfile& profile_at(int profile);
	SaveData load_from_json(const char* json_path) const;
	bool write_save_file(const SaveData& data) const;

	static bool file_exists(const std::string& path);
};
