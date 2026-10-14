#include "game/pch.h"
#include "game/states/menu_level_settings.h"
#include "game/input_slots.h"
using namespace labrador;

std::optional<float> MenuLevelSettings::duration() const
{
	return this->duration_;
}

void MenuLevelSettings::set_duration(std::optional<float> seconds)
{
	if (seconds && (!std::isfinite(*seconds) || *seconds <= 0.0f))
	{
		throw std::invalid_argument("Match duration must be finite and positive");
	}
	this->duration_ = seconds;
}

std::vector<MenuPlayerSettings> MenuLevelSettings::player_settings() const
{
	return this->player_settings_;
}

void MenuLevelSettings::set_player_count(int player_count)
{
	if (player_count < 1 || player_count > input_slots::pad_count)
	{
		throw std::invalid_argument("Player count must be between 1 and 4");
	}
	// The vector is the count's single source of truth, including if growth throws.
	this->player_settings_.resize(static_cast<size_t>(player_count));
}

int MenuLevelSettings::player_count() const
{
	return static_cast<int>(this->player_settings_.size());
}

void MenuLevelSettings::set_stage(const std::string& stage)
{
	this->stage_ = stage;
}

const std::string& MenuLevelSettings::stage() const
{
	return this->stage_;
}

PlayerVisualMode MenuLevelSettings::workshop_visual_mode() const
{
	return this->workshop_visual_mode_;
}

void MenuLevelSettings::set_workshop_visual_mode(PlayerVisualMode mode)
{
	if (mode != PlayerVisualMode::original && mode != PlayerVisualMode::continuous &&
		mode != PlayerVisualMode::authored)
	{
		throw std::invalid_argument("Invalid workshop visual comparison");
	}
	this->workshop_visual_mode_ = mode;
}

ScreenLayout MenuLevelSettings::screen_layout() const
{
	return this->screen_layout_;
}

void MenuLevelSettings::set_screen_layout(ScreenLayout screen_layout)
{
	this->screen_layout_ = screen_layout;
}

void MenuLevelSettings::set_player_team(int player_index, PlayerTeam team)
{
	this->player_settings_.at(static_cast<size_t>(player_index)).team = team;
}
PlayerTeam MenuLevelSettings::player_team(int player_index) const
{
	return this->player_settings_.at(static_cast<size_t>(player_index)).team;
}
void MenuLevelSettings::set_player_weapon(int player_index, WeaponType weapon)
{
	this->player_settings_.at(static_cast<size_t>(player_index)).weapon = weapon;
}
WeaponType MenuLevelSettings::player_weapon(int player_index) const
{
	return this->player_settings_.at(static_cast<size_t>(player_index)).weapon;
}

void MenuLevelSettings::begin_input_claims(int initiating_slot)
{
	if (this->player_settings_.empty() ||
		initiating_slot < 0 || initiating_slot >= input_slots::count)
	{
		throw std::invalid_argument("Cannot bind the player roster to these inputs");
	}
	for (MenuPlayerSettings& player : this->player_settings_)
	{
		player.num = -1;
	}
	this->player_settings_.front().num = initiating_slot;
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
	this->player_settings_[static_cast<size_t>(player)].num = -1;
	return true;
}

bool MenuLevelSettings::input_roster_complete() const
{
	return !this->player_settings_.empty() && std::all_of(
		this->player_settings_.begin(), this->player_settings_.end(),
		[](const MenuPlayerSettings& player) { return player.num >= 0; });
}

int MenuLevelSettings::player_index_for_slot(int slot) const
{
	if (slot < 0 || slot >= input_slots::count)
	{
		return -1;
	}
	for (int player = 0; player < this->player_count(); ++player)
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
	const MenuPlayerSettings& player =
		this->player_settings_.at(static_cast<size_t>(player_index));
	return L"P" + std::to_wstring(player_index + 1) + L" - " +
		input_slots::device_name(player.num);
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
