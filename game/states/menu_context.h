#pragma once

#include "game/game_data.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"

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
private:
	GameData* data_ = nullptr;
	MenuInput* input_ = nullptr;
	MenuLevelSettings* level_settings_ = nullptr;
};
