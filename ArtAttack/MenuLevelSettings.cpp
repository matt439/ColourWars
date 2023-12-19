#include "pch.h"
#include "MenuLevelSettings.h"


void MenuLevelSettings::set_player_count(int player_count)
{
	this->_player_count = player_count;
	int vector_size = static_cast<int>(this->_player_settings.size());
	if (vector_size < player_count)
	{
		this->_player_settings.resize(player_count);
		for (int i = vector_size; i < player_count; i++)
		{
			this->_player_settings[i] = menu_player_settings();
		}
	}
	else if (vector_size > player_count)
	{
		this->_player_settings.resize(player_count);
	}
}