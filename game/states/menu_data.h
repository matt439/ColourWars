#pragma once

#include "game/game_data.h"
#include "game/states/menu_input.h"

class MenuData : public GameData
{
public:
	explicit MenuData(const GameData* game_data);
	MenuData(const GameData* game_data, MenuInput* input);
	MenuInput* input() const;
	void set_input(MenuInput* input);
	MenuData* menu_data();
private:
	MenuInput* input_ = nullptr;
};
