#ifndef RESULTSMENUDATA_H
#define RESULTSMENUDATA_H

#include "MenuData.h"
#include "level_end_info.h"
#include "results_menu_action.h"

class ResultsMenuData : public MenuData
{
private:
	level_end_info _level_end_info = level_end_info();
	results_menu_action* _action = nullptr;
public:
	ResultsMenuData(GameData* game_data, MenuInput* input, level_end_info info,
		results_menu_action* action) :
		MenuData(game_data, input),
		_level_end_info(info),
		_action(action) {}

	level_end_info get_level_end_info() { return _level_end_info; }
	void set_level_end_info(level_end_info level_end_info) { this->_level_end_info = level_end_info; }
	results_menu_action* get_action() { return this->_action; }
	void set_action(results_menu_action* action) { this->_action = action; }
};

#endif // !RESULTSMENUDATA_H
