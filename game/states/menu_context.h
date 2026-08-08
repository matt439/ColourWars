#pragma once

#include "game/game_data.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"

// What every menu page in this game is handed.
//
// IT REPLACES FIVE CLASSES. MenuData and the four that derived from it -
// MainMenuData, PauseMenuData, ResultsMenuData, EndMenuData - and between them
// they carried exactly three things: the services, the pad reader the whole
// screen shares, and the level settings being assembled. Everything else on
// them was an out-parameter for a result, which is what the state stack returns
// now, or a per-page constant, which is what a page's own constructor is for.
//
// All five inherited GameData, so every one of them was also a *copy* of the
// service bag - "a menu's data IS-A game data" is not a sentence anyone meant
// to write. This holds a pointer to the one the shell made.
//
// Not a StateContext, which is the engine's stack. This is the game's own
// noun: the screen's shared state, owned by whichever state owns the screen -
// GameMenu for the main menu, GameLevel for the three that appear over a
// match - and outliving every page put on the stack above it.
class MenuContext
{
public:
	MenuContext(GameData* data, MenuInput* input,
		MenuLevelSettings* level_settings);

	GameData* data() const;
	MenuInput* input() const;

	// The settings this screen is assembling, or - over a match - the ones it
	// was built from. Never null: a menu with nothing to say about a level is a
	// menu that has no reason to be in this game.
	MenuLevelSettings* level_settings() const;
private:
	GameData* data_ = nullptr;
	MenuInput* input_ = nullptr;
	MenuLevelSettings* level_settings_ = nullptr;
};
