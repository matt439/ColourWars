#pragma once

#include "game/states/menu_data.h"
#include "game/states/end_menu_action.h"

class EndMenuData : public MenuData
{
public:
	EndMenuData(GameData* game_data, MenuInput* input, EndMenuAction* action);

	EndMenuAction* get_action() const;
	void set_action(EndMenuAction* action);

private:
	EndMenuAction* action_ = nullptr;
};
