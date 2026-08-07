#pragma once

#include "game/states/menu_data.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"

class MainMenuData : public MenuData
{
public:
	explicit MainMenuData(GameData* game_data);
	MainMenuData(GameData* game_data, MenuInput* input,
		MenuLevelSettings* settings, bool* ready_to_load);

	MenuLevelSettings* level_settings() const;
	void set_level_settings(MenuLevelSettings* level_settings);
	bool* is_ready_to_load_level() const;
	void set_is_ready_to_load_level(bool* is_ready_to_load_level);
private:
	MenuLevelSettings* menu_level_settings_ = nullptr;
	bool* is_ready_to_load_level_ = nullptr;
};
