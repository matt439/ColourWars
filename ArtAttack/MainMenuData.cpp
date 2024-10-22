#include "pch.h"
#include "MainMenuData.h"

MainMenuData::MainMenuData(GameData* game_data) :
	MenuData(game_data)
{

}

MainMenuData::MainMenuData(GameData* game_data, MenuInput* input,
	MenuLevelSettings* settings, bool* ready_to_load) :
	MenuData(game_data, input),
	_menu_level_settings(settings),
	_is_ready_to_load_level(ready_to_load)
{

}

MenuLevelSettings* MainMenuData::get_level_settings() const
{
	return this->_menu_level_settings;
}

void MainMenuData::set_level_settings(MenuLevelSettings* level_settings)
{
	this->_menu_level_settings = level_settings;
}

bool* MainMenuData::get_is_ready_to_load_level() const
{
	return this->_is_ready_to_load_level;
}

void MainMenuData::set_is_ready_to_load_level(bool* is_ready_to_load_level)
{
	this->_is_ready_to_load_level = is_ready_to_load_level;
}

