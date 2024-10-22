#include "pch.h"
#include "PauseMenuData.h"

PauseMenuData::PauseMenuData(GameData* game_data, MenuInput* input,
	int player_num, pause_menu_action* action) :
	MenuData(game_data, input),
	_player_num(player_num),
	_action(action)
{

}

int PauseMenuData::get_player_num() const
{
	return this->_player_num;
}

void PauseMenuData::set_player_num(int player_num)
{
	this->_player_num = player_num;
}

pause_menu_action* PauseMenuData::get_action() const
{
	return this->_action;
}

void PauseMenuData::set_action(pause_menu_action* action)
{
	this->_action = action;
}