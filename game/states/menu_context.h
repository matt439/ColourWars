#pragma once

#include "game/game_data.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"
#include "game/states/connection_audio.h"
#include <functional>

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
	// GameLevel lends its current music clock to the top covering menu page.
	void set_music_update(std::function<void(float)> update);
	void update_music(float dt);
private:
	GameData* data_ = nullptr;
	MenuInput* input_ = nullptr;
	MenuLevelSettings* level_settings_ = nullptr;
	std::unique_ptr<ConnectionAudio> connection_audio_;
	std::function<void(float)> music_update_;
};
