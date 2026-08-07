#include "game/pch.h"
#include "game/states/results_menu_data.h"

ResultsMenuData::ResultsMenuData(const GameData* game_data, MenuInput* input,
	const LevelEndInfo& info, ResultsMenuAction* action) :
	MenuData(game_data, input),
	level_end_info_(info),
	action_(action)
{

}

LevelEndInfo ResultsMenuData::level_end_info() const
{
	return this->level_end_info_;
}

void ResultsMenuData::set_level_end_info(const LevelEndInfo& level_end_info)
{
	this->level_end_info_ = level_end_info;
}

ResultsMenuAction* ResultsMenuData::action() const
{
	return this->action_;
}

void ResultsMenuData::set_action(ResultsMenuAction* action)
{
	this->action_ = action;
}
