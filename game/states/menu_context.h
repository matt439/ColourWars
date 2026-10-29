#pragma once

#include "game/game_data.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"
#include "game/states/connection_audio.h"
#include <functional>

struct UserStageScan;

// Shared menu services, input, and level settings. Borrows all three.
// Owned by GameMenu or GameLevel, and outlives the pages above that state.
class MenuContext
{
public:
	MenuContext(GameData* data, MenuInput* input,
		MenuLevelSettings* level_settings);

	GameData* data() const;
	MenuInput* input() const;

	// Non-null settings being assembled, or those used to build the active match.
	MenuLevelSettings* level_settings() const;
	// Enabled only by GameLevel; a setup-menu roster is still being edited.
	void reset_connection_audio();
	void update_connection_audio();
	// The screen owner lends its music clock to the top covering menu page.
	void set_music_update(std::function<void(float)> update);
	void update_music(float dt);
	void set_music_suspend(std::function<void()> suspend);
	void suspend_music();
	// ROAD#16: rereads the player's stage folder. Only GameMenu lends one,
	// and only as the main-menu Stage Select opens; otherwise nothing.
	void set_stage_rescan(std::function<void()> rescan);
	void rescan_stages();
	// The main-menu owner's retained result; reading it never scans. Match
	// contexts leave this unset, so no diagnostics page can rescan a series.
	void set_stage_scan(std::function<const UserStageScan*()> scan);
	const UserStageScan* stage_scan() const;
	// Keep the outgoing page visible and ignore further input until its owner
	// finishes the fade. Callbacks capture the state context, never the page.
	void finish_with_music(std::function<void()> finish);
	bool music_exit_pending() const;
	void complete_music_exit();
	void cancel_music_exit();
private:
	GameData* data_ = nullptr;
	MenuInput* input_ = nullptr;
	MenuLevelSettings* level_settings_ = nullptr;
	std::unique_ptr<ConnectionAudio> connection_audio_;
	std::function<void(float)> music_update_;
	std::function<void()> music_suspend_;
	std::function<void()> stage_rescan_;
	std::function<const UserStageScan*()> stage_scan_;
	std::function<void()> music_exit_;
};
