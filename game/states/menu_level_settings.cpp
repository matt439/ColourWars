#include "game/pch.h"
#include "game/states/menu_level_settings.h"
#include "game/input_slots.h"
using namespace labrador;

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

void MenuLevelSettings::set_stage(const std::string& stage)
{
	this->stage_ = stage;
}

const std::string& MenuLevelSettings::stage() const
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

void MenuLevelSettings::begin_input_claims(int initiating_slot)
{
	if (this->player_count_ < 1 || this->player_count_ > input_slots::pad_count ||
		initiating_slot < 0 || initiating_slot >= input_slots::count)
	{
		throw std::invalid_argument("Cannot bind the player roster to these inputs");
	}
	for (MenuPlayerSettings& player : this->player_settings_)
	{
		player.num = -1;
	}
	this->set_player_num(0, initiating_slot);
}

bool MenuLevelSettings::claim_input_slot(int slot)
{
	if (slot < 0 || slot >= input_slots::count ||
		this->player_index_for_slot(slot) != -1)
	{
		return false;
	}
	for (MenuPlayerSettings& player : this->player_settings_)
	{
		if (player.num == -1)
		{
			player.num = slot;
			return true;
		}
	}
	return false;
}

bool MenuLevelSettings::release_input_slot(int slot)
{
	const int player = this->player_index_for_slot(slot);
	if (player <= 0)
	{
		return false;
	}
	this->set_player_num(player, -1);
	return true;
}

bool MenuLevelSettings::input_roster_complete() const
{
	return this->player_count_ > 0 && std::all_of(
		this->player_settings_.begin(), this->player_settings_.end(),
		[](const MenuPlayerSettings& player) { return player.num >= 0; });
}

int MenuLevelSettings::player_index_for_slot(int slot) const
{
	if (slot < 0 || slot >= input_slots::count)
	{
		return -1;
	}
	for (int player = 0; player < this->player_count_; ++player)
	{
		if (this->player_settings_[player].num == slot)
		{
			return player;
		}
	}
	return -1;
}

std::wstring MenuLevelSettings::player_label(int player_index) const
{
	return L"P" + std::to_wstring(player_index + 1) + L" - " +
		input_slots::device_name(this->player_settings_[player_index].num);
}

int MenuLevelSettings::best_of() const
{
	return this->best_of_;
}

void MenuLevelSettings::set_best_of(int best_of)
{
	if (best_of != 1 && best_of != 3 && best_of != 5)
	{
		throw std::invalid_argument("Session length must be 1, 3 or 5");
	}
	this->best_of_ = best_of;
}
