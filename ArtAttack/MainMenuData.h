#ifndef MAINMENUDATA_H
#define MAINMENUDATA_H

#include "MenuData.h"
#include "MenuInput.h"
#include "MenuLevelSettings.h"

class MainMenuData : public MenuData
{
public:
	explicit MainMenuData(GameData* game_data) :
		MenuData(game_data) {}
	MainMenuData(GameData* game_data, MenuInput* input,
		MenuLevelSettings* settings, bool* ready_to_load) :
		MenuData(game_data, input),
		_menu_level_settings(settings),
		_is_ready_to_load_level(ready_to_load) {}

	MenuLevelSettings* get_level_settings() const { return this->_menu_level_settings; }
	void set_level_settings(MenuLevelSettings* level_settings) { this->_menu_level_settings = level_settings; }
	bool* get_is_ready_to_load_level() const { return this->_is_ready_to_load_level; }
	void set_is_ready_to_load_level(bool* is_ready_to_load_level) { this->_is_ready_to_load_level = is_ready_to_load_level; }
private:
	MenuLevelSettings* _menu_level_settings = nullptr;
	bool* _is_ready_to_load_level = nullptr;
};

#endif // !MAINMENUDATA_H
