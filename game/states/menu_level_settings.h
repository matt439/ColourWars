#pragma once

#include <vector>
#include "game/objects/level_mode.h"
#include "game/objects/level_stage.h"
#include "game/objects/wep_type.h"
#include "game/states/menu_player_settings.h"
#include "engine/render/screen_layout.h"

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
	level_mode game_mode_ = level_mode::STANDARD_MODE;
	std::vector<MenuPlayerSettings> player_settings_;
	int player_count_ = 0;
	level_stage stage_ = level_stage::KING_OF_THE_HILL;
	screen_layout screen_layout_ = screen_layout::ONE_PLAYER;
};