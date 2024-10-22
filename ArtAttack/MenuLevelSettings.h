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
	std::vector<MenuPlayerSettings> get_player_settings() const { return this->_player_settings; }
	void set_player_count(int player_count);
	int get_player_count() const { return this->_player_count; }
	void set_game_mode(level_mode game_mode) { this->_game_mode = game_mode; }
	level_mode get_game_mode() const { return this->_game_mode; }
	void set_stage(level_stage stage) { this->_stage = stage; }
	level_stage get_stage() const { return this->_stage; }
	screen_layout get_screen_layout() const { return this->_screen_layout; }
	void set_screen_layout(screen_layout screen_layout) { this->_screen_layout = screen_layout; }

	void set_player_setting(int player_index, MenuPlayerSettings player_settings)
	{
		this->_player_settings[player_index] = player_settings;
	}
	void set_player_team(int player_index, player_team team)
	{
		this->_player_settings[player_index].team = team;
	}
	player_team get_player_team(int player_index) const
	{
		return this->_player_settings[player_index].team;
	}
	void set_player_weapon(int player_index, wep_type weapon)
	{
		this->_player_settings[player_index].weapon = weapon;
	}
	wep_type get_player_weapon(int player_index) const
	{
		return this->_player_settings[player_index].weapon;
	}
	void set_player_num(int player_index, int player_num)
	{
		this->_player_settings[player_index].num = player_num;
	}

private:
	level_mode _game_mode = level_mode::STANDARD_MODE;
	std::vector<MenuPlayerSettings> _player_settings;
	int _player_count = 0;
	level_stage _stage = level_stage::KING_OF_THE_HILL;
	screen_layout _screen_layout = screen_layout::ONE_PLAYER;
};
#endif