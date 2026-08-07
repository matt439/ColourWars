#include "game/pch.h"
#include "game/states/end_menu_data.h"

EndMenuData::EndMenuData(GameData* game_data, MenuInput* input,
	EndMenuAction* action) :
	MenuData(game_data, input),
	action_(action)
{

}

EndMenuAction* EndMenuData::action() const
{
	return this->action_;
}

void EndMenuData::set_action(EndMenuAction* action)
{
	this->action_ = action;
}