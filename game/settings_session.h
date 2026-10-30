#pragma once

#include "game/input_bindings.h"
#include "game/input_slots.h"
#include "game/play_settings.h"
#include <array>
#include <string>

class Save;

// This run's preferences: the Save's, with each worn player profile laid over
// the pad wearing it. Which pad wears which profile belongs to the run and is
// never written, so it lives here rather than in the Save; a pad starts each
// launch as itself. Every reader of the settings in effect (matches, menus,
// sound groups) reads them here; the Save holds only what the file holds.
//
// Borrows the Save, which must outlive it. Profile list changes that move
// indices (deleting one) go through the session so the worn indices follow.
class SettingsSession
{
public:
	// Starts with every pad as itself.
	explicit SettingsSession(Save& save);
	SettingsSession(const SettingsSession&) = delete;
	SettingsSession& operator=(const SettingsSession&) = delete;

	Save& save() const { return *this->save_; }

	// Rereads the save file; profiles may have changed underneath, so every
	// pad starts as itself again.
	void load_save_file();

	// The effective preferences: a pad slot wearing a profile shows that
	// profile's sticks, rumble, comfort switches and buttons in place of its
	// own. The references stay valid, and current, for the session's lifetime.
	const PlaySettings& play_settings() const;
	const InputBindings& input_bindings() const;
	// Takes effective preferences. A worn slot's per-pad values go to its
	// profile; the slot's own values stay as they were.
	bool set_play_settings_and_save(const PlaySettings& settings);
	// Like play preferences, a valid choice becomes active before the write;
	// failure keeps it active and leaves the previous disk file intact. A
	// worn slot's buttons go to its profile.
	bool set_input_bindings_and_save(const InputBindings& bindings);

	// Player profiles (ROAD#21). The profile `slot` wears, or -1. Non-pad
	// slots wear none.
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
	// The pad wearing it goes back to its own settings; later wearers'
	// indices shift down with the list. Throws for an unknown index.
	bool delete_profile_and_save(int profile);
	// Copies each worn pad's chosen Boost into its profile, writing only
	// when one changed. `boosts` is per pad slot; -1 leaves a slot alone.
	bool remember_profile_boosts_and_save(const std::array<int, input_slots::pad_count>& boosts);

private:
	Save* save_;
	std::array<int, input_slots::pad_count> worn_{ -1, -1, -1, -1 };
	// The save's settings with the worn profiles laid over their slots.
	// Rebuilt in place, never replaced, because matches and pages borrow them.
	PlaySettings play_;
	InputBindings bindings_;
	void refresh_effective();
};
