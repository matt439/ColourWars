#include "game/pch.h"
#include "game/states/end_menu_data.h"

EndMenuData::EndMenuData(GameData* game_data, MenuInput* input,
	end_menu_action* action) :
	MenuData(game_data, input),
	action_(action)
{

}

end_menu_action* EndMenuData::get_action() const
{
	return this->action_;
}

void EndMenuData::set_action(end_menu_action* action)
{
	this->action_ = action;
}