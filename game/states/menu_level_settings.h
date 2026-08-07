#pragma once

#include <vector>
#include "game/objects/level_mode.h"
#include "game/objects/level_stage.h"
#include "game/objects/weapon_type.h"
#include "game/states/menu_player_settings.h"
#include "engine/render/screen_layout.h"

class MenuLevelSettings
{
public:
	MenuLevelSettings() = default;
	std::vector<MenuPlayerSettings> player_settings() const;
	void set_player_count(int player_count);
	int player_count() const;
	void set_game_mode(LevelMode game_mode);
	LevelMode game_mode() const;
	void set_stage(LevelStage stage);
	LevelStage stage() const;
	ScreenLayout screen_layout() const;
	void set_screen_layout(ScreenLayout ScreenLayout);

	void set_player_setting(int player_index, MenuPlayerSettings player_settings);
	void set_player_team(int player_index, PlayerTeam team);
	PlayerTeam player_team(int player_index) const;
	void set_player_weapon(int player_index, WeaponType weapon);
	WeaponType player_weapon(int player_index) const;
	void set_player_num(int player_index, int player_num);

private:
	LevelMode game_mode_ = LevelMode::standard_mode;
	std::vector<MenuPlayerSettings> player_settings_;
	int player_count_ = 0;
	LevelStage stage_ = LevelStage::king_of_the_hill;
	ScreenLayout screen_layout_ = ScreenLayout::one_player;
};