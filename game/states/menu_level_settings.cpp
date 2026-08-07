#include "game/pch.h"
#include "game/states/menu_level_settings.h"

std::vector<MenuPlayerSettings> MenuLevelSettings::get_player_settings() const
{
	return this->player_settings_;
}

void MenuLevelSettings::set_player_count(int player_count)
{
	this->player_count_ = player_count;
	int vector_size = static_cast<int>(this->player_settings_.size());
	if (vector_size < player_count)
	{
		this->player_settings_.resize(player_count);
		for (int i = vector_size; i < player_count; i++)
		{
			this->player_settings_[i] = MenuPlayerSettings();
		}
	}
	else if (vector_size > player_count)
	{
		this->player_settings_.resize(player_count);
	}
}

int MenuLevelSettings::get_player_count() const
{
	return this->player_count_;
}

void MenuLevelSettings::set_game_mode(LevelMode game_mode)
{
	this->game_mode_ = game_mode;
}

LevelMode MenuLevelSettings::get_game_mode() const
{
	return this->game_mode_;
}

void MenuLevelSettings::set_stage(LevelStage stage)
{
	this->stage_ = stage;
}

LevelStage MenuLevelSettings::get_stage() const
{
	return this->stage_;
}

ScreenLayout MenuLevelSettings::get_screen_layout() const
{
	return this->screen_layout_;
}

void MenuLevelSettings::set_screen_layout(ScreenLayout ScreenLayout)
{
	this->screen_layout_ = ScreenLayout;
}

void MenuLevelSettings::set_player_setting(int player_index, MenuPlayerSettings player_settings)
{
	this->player_settings_[player_index] = player_settings;
}
void MenuLevelSettings::set_player_team(int player_index, PlayerTeam team)
{
	this->player_settings_[player_index].team = team;
}
PlayerTeam MenuLevelSettings::get_player_team(int player_index) const
{
	return this->player_settings_[player_index].team;
}
void MenuLevelSettings::set_player_weapon(int player_index, WeaponType weapon)
{
	this->player_settings_[player_index].weapon = weapon;
}
WeaponType MenuLevelSettings::get_player_weapon(int player_index) const
{
	return this->player_settings_[player_index].weapon;
}
void MenuLevelSettings::set_player_num(int player_index, int player_num)
{
	this->player_settings_[player_index].num = player_num;
}