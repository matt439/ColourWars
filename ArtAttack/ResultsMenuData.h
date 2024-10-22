#ifndef RESULTSMENUDATA_H
#define RESULTSMENUDATA_H

#include "MenuData.h"
#include "LevelEndInfo.h"
#include "results_menu_action.h"

class ResultsMenuData : public MenuData
{
private:
	LevelEndInfo _level_end_info = LevelEndInfo();
	results_menu_action* _action = nullptr;
public:
	ResultsMenuData(GameData* game_data, MenuInput* input, LevelEndInfo info,
		results_menu_action* action) :
		MenuData(game_data, input),
		_level_end_info(info),
		_action(action) {}

	LevelEndInfo get_level_end_info() { return _level_end_info; }
	void set_level_end_info(LevelEndInfo level_end_info) { this->_level_end_info = level_end_info; }
	results_menu_action* get_action() { return this->_action; }
	void set_action(results_menu_action* action) { this->_action = action; }
};

#endif // !RESULTSMENUDATA_H
