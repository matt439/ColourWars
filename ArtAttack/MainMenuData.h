#ifndef MAINMENUDATA_H
#define MAINMENUDATA_H

#include "MenuData.h"
#include "MenuInput.h"
#include "MenuLevelSettings.h"

class MainMenuData : public MenuData
{
public:
	explicit MainMenuData(GameData* game_data);
	MainMenuData(GameData* game_data, MenuInput* input,
		MenuLevelSettings* settings, bool* ready_to_load);

	MenuLevelSettings* get_level_settings() const;
	void set_level_settings(MenuLevelSettings* level_settings);
	bool* get_is_ready_to_load_level() const;
	void set_is_ready_to_load_level(bool* is_ready_to_load_level);
private:
	MenuLevelSettings* _menu_level_settings = nullptr;
	bool* _is_ready_to_load_level = nullptr;
};

#endif // !MAINMENUDATA_H
