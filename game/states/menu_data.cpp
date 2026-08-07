#include "game/pch.h"
#include "game/states/menu_data.h"

MenuData::MenuData(const GameData* game_data) :
	GameData(game_data)
{

}

MenuData::MenuData(const GameData* game_data, MenuInput* input) :
	GameData(game_data), input_(input)
{

}

MenuInput* MenuData::get_input() const
{
	return this->input_;
}

void MenuData::set_input(MenuInput* input)
{
	this->input_ = input;
}

MenuData* MenuData::get_menu_data()
{
	return this;
}