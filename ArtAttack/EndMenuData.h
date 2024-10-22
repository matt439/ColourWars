#ifndef ENDMENUDATA_H
#define ENDMENUDATA_H

#include "MenuData.h"
#include "end_menu_action.h"

class EndMenuData : public MenuData
{
public:
	EndMenuData(GameData* game_data, MenuInput* input, end_menu_action* action) :
		MenuData(game_data, input),
		_action(action) {}

	end_menu_action* get_action() { return this->_action; }
	void set_action(end_menu_action* action) { this->_action = action; }

private:
	end_menu_action* _action = nullptr;
};

#endif // !ENDMENUDATA_H
