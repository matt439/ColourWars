#ifndef MENUDATA_H
#define MENUDATA_H

#include "GameData.h"
#include "MenuInput.h"

class MenuData : public GameData
{
private:
	MenuInput* _input = nullptr;
public:
	MenuData(GameData* game_data) : GameData(game_data) {}
	MenuData(GameData* game_data, MenuInput* input) :
		GameData(game_data), _input(input) {}
	MenuInput* get_input() { return this->_input; }
	void set_input(MenuInput* input) { this->_input = input; }
	MenuData* get_menu_data() { return this; }
};
#endif // !MENUDATA_H
