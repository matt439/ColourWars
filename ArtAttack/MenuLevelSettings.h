#ifndef MENULEVELSETTINGS_H
#define MENULEVELSETTINGS_H
#include <vector>
#include "level_mode.h"
#include "level_stage.h"
#include "wep_type.h"
#include "MenuPlayerSettings.h"
#include "screen_layout.h"

class MenuLevelSettings
{
public:
	MenuLevelSettings() = default;
	std::vector<MenuPlayerSettings> get_player_settings() const;
	void set_player_count(int player_count);
	int get_player_count() const;
	void set_game_mode(level_mode game_mode);
	level_mode get_game_mode() const;
	void set_stage(level_stage stage);
	level_stage get_stage() const;
	screen_layout get_screen_layout() const;
	void set_screen_layout(screen_layout screen_layout);

	void set_player_setting(int player_index, MenuPlayerSettings player_settings);
	void set_player_team(int player_index, player_team team);
	player_team get_player_team(int player_index) const;
	void set_player_weapon(int player_index, wep_type weapon);
	wep_type get_player_weapon(int player_index) const;
	void set_player_num(int player_index, int player_num);

private:
	level_mode _game_mode = level_mode::STANDARD_MODE;
	std::vector<MenuPlayerSettings> _player_settings;
	int _player_count = 0;
	level_stage _stage = level_stage::KING_OF_THE_HILL;
	screen_layout _screen_layout = screen_layout::ONE_PLAYER;
};
#endif