#include "game/pch.h"
#include "game/states/menu_level_settings.h"
using namespace artattack;

std::vector<MenuPlayerSettings> MenuLevelSettings::player_settings() const
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

int MenuLevelSettings::player_count() const
{
	return this->player_count_;
}

void MenuLevelSettings::set_game_mode(LevelMode game_mode)
{
	this->game_mode_ = game_mode;
}

LevelMode MenuLevelSettings::game_mode() const
{
	return this->game_mode_;
}

void MenuLevelSettings::set_stage(LevelStage stage)
{
	this->stage_ = stage;
}

LevelStage MenuLevelSettings::stage() const
{
	return this->stage_;
}

ScreenLayout MenuLevelSettings::screen_layout() const
{
	return this->screen_layout_;
}

void MenuLevelSettings::set_screen_layout(ScreenLayout screen_layout)
{
	this->screen_layout_ = screen_layout;
}

void MenuLevelSettings::set_player_setting(int player_index, MenuPlayerSettings player_settings)
{
	this->player_settings_[player_index] = player_settings;
}
void MenuLevelSettings::set_player_team(int player_index, PlayerTeam team)
{
	this->player_settings_[player_index].team = team;
}
PlayerTeam MenuLevelSettings::player_team(int player_index) const
{
	return this->player_settings_[player_index].team;
}
void MenuLevelSettings::set_player_weapon(int player_index, WeaponType weapon)
{
	this->player_settings_[player_index].weapon = weapon;
}
WeaponType MenuLevelSettings::player_weapon(int player_index) const
{
	return this->player_settings_[player_index].weapon;
}
void MenuLevelSettings::set_player_num(int player_index, int player_num)
{
	this->player_settings_[player_index].num = player_num;
}