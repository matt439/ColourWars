#pragma once

#include "game/states/menu_data.h"
#include "game/objects/level_end_info.h"
#include "game/states/results_menu_action.h"

class ResultsMenuData : public MenuData
{
public:
	ResultsMenuData(const GameData* game_data, MenuInput* input,
		const LevelEndInfo& info, results_menu_action* action);

	LevelEndInfo get_level_end_info() const;
	void set_level_end_info(const LevelEndInfo& level_end_info);
	results_menu_action* get_action() const;
	void set_action(results_menu_action* action);

private:
	LevelEndInfo _level_end_info = LevelEndInfo();
	results_menu_action* _action = nullptr;
};
