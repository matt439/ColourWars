#include "pch.h"
#include "MenuData.h"

MenuData::MenuData(const GameData* game_data) :
	GameData(game_data)
{

}

MenuData::MenuData(const GameData* game_data, MenuInput* input) :
	GameData(game_data), _input(input)
{

}

MenuInput* MenuData::get_input() const
{
	return this->_input;
}

void MenuData::set_input(MenuInput* input)
{
	this->_input = input;
}

MenuData* MenuData::get_menu_data()
{
	return this;
}