#pragma once

#include "game/game_data.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"
#include "game/states/connection_audio.h"

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
private:
	GameData* data_ = nullptr;
	MenuInput* input_ = nullptr;
	MenuLevelSettings* level_settings_ = nullptr;
	std::unique_ptr<ConnectionAudio> connection_audio_;
};
