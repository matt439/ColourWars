#include "pch.h"
#include "EndMenuData.h"

EndMenuData::EndMenuData(GameData* game_data, MenuInput* input,
	end_menu_action* action) :
	MenuData(game_data, input),
	_action(action)
{

}

end_menu_action* EndMenuData::get_action() const
{
	return this->_action;
}

void EndMenuData::set_action(end_menu_action* action)
{
	this->_action = action;
}