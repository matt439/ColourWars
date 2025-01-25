#include "pch.h"
#include "MenuLevelSettings.h"

std::vector<MenuPlayerSettings> MenuLevelSettings::get_player_settings() const
{
	return this->_player_settings;
}

void MenuLevelSettings::set_player_count(int player_count)
{
	this->_player_count = player_count;
	int vector_size = static_cast<int>(this->_player_settings.size());
	if (vector_size < player_count)
	{
		this->_player_settings.resize(player_count);
		for (int i = vector_size; i < player_count; i++)
		{
			this->_player_settings[i] = MenuPlayerSettings();
		}
	}
	else if (vector_size > player_count)
	{
		this->_player_settings.resize(player_count);
	}
}

int MenuLevelSettings::get_player_count() const
{
	return this->_player_count;
}

void MenuLevelSettings::set_game_mode(level_mode game_mode)
{
	this->_game_mode = game_mode;
}

level_mode MenuLevelSettings::get_game_mode() const
{
	return this->_game_mode;
}

void MenuLevelSettings::set_stage(level_stage stage)
{
	this->_stage = stage;
}

level_stage MenuLevelSettings::get_stage() const
{
	return this->_stage;
}

screen_layout MenuLevelSettings::get_screen_layout() const
{
	return this->_screen_layout;
}

void MenuLevelSettings::set_screen_layout(screen_layout screen_layout)
{
	this->_screen_layout = screen_layout;
}

void MenuLevelSettings::set_player_setting(int player_index, MenuPlayerSettings player_settings)
{
	this->_player_settings[player_index] = player_settings;
}
void MenuLevelSettings::set_player_team(int player_index, player_team team)
{
	this->_player_settings[player_index].team = team;
}
player_team MenuLevelSettings::get_player_team(int player_index) const
{
	return this->_player_settings[player_index].team;
}
void MenuLevelSettings::set_player_weapon(int player_index, wep_type weapon)
{
	this->_player_settings[player_index].weapon = weapon;
}
wep_type MenuLevelSettings::get_player_weapon(int player_index) const
{
	return this->_player_settings[player_index].weapon;
}
void MenuLevelSettings::set_player_num(int player_index, int player_num)
{
	this->_player_settings[player_index].num = player_num;
}