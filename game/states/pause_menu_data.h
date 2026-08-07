#ifndef PAUSEMENUDATA_H
#define PAUSEMENUDATA_H

#include "game/states/menu_data.h"
#include "game/states/pause_menu_action.h"

class PauseMenuData : public MenuData
{
public:
	PauseMenuData(GameData* game_data, MenuInput* input, int player_num,
		pause_menu_action* action);

	int get_player_num() const;
	void set_player_num(int player_num);
	pause_menu_action* get_action() const;
	void set_action(pause_menu_action* action);
private:
	int _player_num = -1;
	pause_menu_action* _action = nullptr;
};

#endif // !PAUSEMENUDATE_H
