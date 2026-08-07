#include "game/pch.h"
#include "game/states/main_menu_data.h"

MainMenuData::MainMenuData(GameData* game_data) :
	MenuData(game_data)
{

}

MainMenuData::MainMenuData(GameData* game_data, MenuInput* input,
	MenuLevelSettings* settings, bool* ready_to_load) :
	MenuData(game_data, input),
	menu_level_settings_(settings),
	is_ready_to_load_level_(ready_to_load)
{

}

MenuLevelSettings* MainMenuData::level_settings() const
{
	return this->menu_level_settings_;
}

void MainMenuData::set_level_settings(MenuLevelSettings* level_settings)
{
	this->menu_level_settings_ = level_settings;
}

bool* MainMenuData::is_ready_to_load_level() const
{
	return this->is_ready_to_load_level_;
}

void MainMenuData::set_is_ready_to_load_level(bool* is_ready_to_load_level)
{
	this->is_ready_to_load_level_ = is_ready_to_load_level;
}

