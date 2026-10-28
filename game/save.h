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
	// The effective preferences: a pad slot wearing a profile shows that
	// profile's sticks, rumble, comfort switches and buttons in place of its
	// own. The reference stays valid, and current, for the Save's lifetime.
	const PlaySettings& play_settings() const;
	// Takes effective preferences. A worn slot's per-pad values go to its
	// profile; the slot's own values stay as they were.
	bool set_play_settings_and_save(const PlaySettings& settings);
	const InputBindings& input_bindings() const;
	// Like play preferences, a valid choice becomes active before the write;
	// failure keeps it active and leaves the previous disk file intact. A
	// worn slot's buttons go to its profile.
	bool set_input_bindings_and_save(const InputBindings& bindings);

	// Player profiles (ROAD#21). Which pad wears which profile is for this
	// run only and is never written: a pad starts each launch as itself.
	const PlayerProfiles& profiles() const;
	// The profile `slot` wears, or -1. Non-pad slots wear none.
	int worn_profile(int slot) const;
	// The pad slot wearing `profile`, or -1.
	int profile_wearer(int profile) const;
	// -1 takes the profile off. One profile is worn by one pad at a time, so
	// a profile another pad wears is refused (false); so are non-pad slots
	// and unknown profiles. Returns true when the slot now wears `profile`.
	bool wear_profile(int slot, int profile);
	// The next profile `slot` may wear after its current one, stepping by +1
	// or -1 through "none" (-1) and skipping profiles other pads wear.
	int next_wearable_profile(int slot, int step) const;
	// Adds a profile named `name` carrying `slot`'s effective settings (the
	// defaults for a non-pad slot). Throws invalid_argument for a bad or
	// taken name, or when full. Returns the new index; the write's outcome
	// goes to `saved`, and the profile stays for this run either way.
	int add_profile_and_save(const std::string& name, int slot, bool& saved);
	// Throws invalid_argument for an unknown index or a bad or taken name.
	bool rename_profile_and_save(int profile, const std::string& name);
	// Throws invalid_argument outside handicaps::valid or for an unknown index.
	bool set_profile_boost_and_save(int profile, int boost);
	// The pad wearing it goes back to its own settings; later wearers'
	// indices shift down with the list. Throws for an unknown index.
	bool delete_profile_and_save(int profile);
	// Copies each worn pad's chosen Boost into its profile, writing only
	// when one changed. `boosts` is per pad slot; -1 leaves a slot alone.
	bool remember_profile_boosts_and_save(const std::array<int, input_slots::pad_count>& boosts);
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
	std::array<int, input_slots::pad_count> worn_{ -1, -1, -1, -1 };
	// save_data_ with the worn profiles laid over their slots. Rebuilt in
	// place, never replaced, because matches and pages borrow them.
	PlaySettings play_;
	InputBindings bindings_;
	void refresh_effective();
	void load_save_data();
	SaveData load_from_json(const char* json_path) const;
	bool write_save_file(const SaveData& data) const;

	static bool file_exists(const std::string& path);
};
