#include "pch.h"
#include "ResultsMenuData.h"

ResultsMenuData::ResultsMenuData(const GameData* game_data, MenuInput* input,
	const LevelEndInfo& info, results_menu_action* action) :
	MenuData(game_data, input),
	_level_end_info(info),
	_action(action)
{

}

LevelEndInfo ResultsMenuData::get_level_end_info() const
{
	return this->_level_end_info;
}

void ResultsMenuData::set_level_end_info(const LevelEndInfo& level_end_info)
{
	this->_level_end_info = level_end_info;
}

results_menu_action* ResultsMenuData::get_action() const
{
	return this->_action;
}

void ResultsMenuData::set_action(results_menu_action* action)
{
	this->_action = action;
}
