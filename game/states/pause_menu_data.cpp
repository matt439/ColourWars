#include "game/pch.h"
#include "game/states/pause_menu_data.h"
using namespace artattack;

PauseMenuData::PauseMenuData(GameData* game_data, MenuInput* input,
	int player_num, PauseMenuAction* action) :
	MenuData(game_data, input),
	player_num_(player_num),
	action_(action)
{

}

int PauseMenuData::player_num() const
{
	return this->player_num_;
}

void PauseMenuData::set_player_num(int player_num)
{
	this->player_num_ = player_num;
}

PauseMenuAction* PauseMenuData::action() const
{
	return this->action_;
}

void PauseMenuData::set_action(PauseMenuAction* action)
{
	this->action_ = action;
}