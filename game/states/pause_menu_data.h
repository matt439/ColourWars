#pragma once

#include "game/states/menu_data.h"
#include "game/states/pause_menu_action.h"

class PauseMenuData : public MenuData
{
public:
	PauseMenuData(GameData* game_data, MenuInput* input, int player_num,
		PauseMenuAction* action);

	int player_num() const;
	void set_player_num(int player_num);
	PauseMenuAction* action() const;
	void set_action(PauseMenuAction* action);
private:
	int player_num_ = -1;
	PauseMenuAction* action_ = nullptr;
};
