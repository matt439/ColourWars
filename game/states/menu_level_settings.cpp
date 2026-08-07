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

void MenuLevelSettings::set_game_mode(level_mode game_mode)
{
	this->game_mode_ = game_mode;
}

level_mode MenuLevelSettings::get_game_mode() const
{
	return this->game_mode_;
}

void MenuLevelSettings::set_stage(level_stage stage)
{
	this->stage_ = stage;
}

level_stage MenuLevelSettings::get_stage() const
{
	return this->stage_;
}

screen_layout MenuLevelSettings::get_screen_layout() const
{
	return this->screen_layout_;
}

void MenuLevelSettings::set_screen_layout(screen_layout screen_layout)
{
	this->screen_layout_ = screen_layout;
}

void MenuLevelSettings::set_player_setting(int player_index, MenuPlayerSettings player_settings)
{
	this->player_settings_[player_index] = player_settings;
}
void MenuLevelSettings::set_player_team(int player_index, player_team team)
{
	this->player_settings_[player_index].team = team;
}
player_team MenuLevelSettings::get_player_team(int player_index) const
{
	return this->player_settings_[player_index].team;
}
void MenuLevelSettings::set_player_weapon(int player_index, wep_type weapon)
{
	this->player_settings_[player_index].weapon = weapon;
}
wep_type MenuLevelSettings::get_player_weapon(int player_index) const
{
	return this->player_settings_[player_index].weapon;
}
void MenuLevelSettings::set_player_num(int player_index, int player_num)
{
	this->player_settings_[player_index].num = player_num;
}