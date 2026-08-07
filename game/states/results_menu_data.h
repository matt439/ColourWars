#pragma once

#include "game/states/menu_data.h"
#include "game/objects/level_end_info.h"
#include "game/states/results_menu_action.h"

class ResultsMenuData : public MenuData
{
public:
	ResultsMenuData(const GameData* game_data, MenuInput* input,
		const LevelEndInfo& info, ResultsMenuAction* action);

	LevelEndInfo get_level_end_info() const;
	void set_level_end_info(const LevelEndInfo& level_end_info);
	ResultsMenuAction* get_action() const;
	void set_action(ResultsMenuAction* action);

private:
	LevelEndInfo level_end_info_ = LevelEndInfo();
	ResultsMenuAction* action_ = nullptr;
};
