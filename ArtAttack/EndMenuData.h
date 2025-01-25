#ifndef ENDMENUDATA_H
#define ENDMENUDATA_H

#include "MenuData.h"
#include "end_menu_action.h"

class EndMenuData : public MenuData
{
public:
	EndMenuData(GameData* game_data, MenuInput* input, end_menu_action* action);

	end_menu_action* get_action() const;
	void set_action(end_menu_action* action);

private:
	end_menu_action* _action = nullptr;
};

#endif // !ENDMENUDATA_H
