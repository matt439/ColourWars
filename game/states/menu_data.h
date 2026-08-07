#ifndef MENUDATA_H
#define MENUDATA_H

#include "game/game_data.h"
#include "game/states/menu_input.h"

class MenuData : public GameData
{
public:
	explicit MenuData(const GameData* game_data);
	MenuData(const GameData* game_data, MenuInput* input);
	MenuInput* get_input() const;
	void set_input(MenuInput* input);
	MenuData* get_menu_data();
private:
	MenuInput* _input = nullptr;
};

#endif // !MENUDATA_H
