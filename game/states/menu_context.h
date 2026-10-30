#pragma once

#include "game/game_data.h"
#include "game/states/menu_input.h"
#include "game/match_setup.h"
#include "game/states/connection_audio.h"
#include "game/states/menu_owner.h"
#include <functional>

// Shared menu services, input, and level settings. Borrows all four.
// Owned by GameMenu or GameLevel, and outlives the pages above that state.
class MenuContext
{
public:
	// `owner` is the state whose music and stage folder the pages reach; null
	// leaves the pages silent, with nothing to rescan.
	MenuContext(GameData* data, MenuInput* input,
		MatchSetup* level_settings, MenuOwner* owner = nullptr);

	// The states pass themselves at construction; a probe re-points the
	// context at a stand-in owner, or at none.
	MenuOwner* owner() const;
	void set_owner(MenuOwner* owner);

	GameData* data() const;
	MenuInput* input() const;

	// Non-null settings being assembled, or those used to build the active match.
	MatchSetup* level_settings() const;
	// Enabled only by GameLevel; a setup-menu roster is still being edited.
	void reset_connection_audio();
	void update_connection_audio();
	// The screen owner lends its music clock to the top covering menu page.
	void update_music(float dt);
	void suspend_music();
	// ROAD#16, through the owner; nothing unless GameMenu lends a stage folder.
	void rescan_stages();
	const UserStageScan* stage_scan() const;
	// Keep the outgoing page visible and ignore further input until its owner
	// finishes the fade. Callbacks capture the state context, never the page.
	// With no owner the callback runs at once.
	void finish_with_music(std::function<void()> finish);
	bool music_exit_pending() const;
	void complete_music_exit();
	void cancel_music_exit();
private:
	GameData* data_ = nullptr;
	MenuInput* input_ = nullptr;
	MatchSetup* level_settings_ = nullptr;
	std::unique_ptr<ConnectionAudio> connection_audio_;
	MenuOwner* owner_ = nullptr;
	// The one pending page exit, run once the owner's fade finishes.
	std::function<void()> music_exit_;
};
