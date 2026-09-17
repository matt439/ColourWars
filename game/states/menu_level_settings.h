#pragma once

#include <string>
#include <vector>
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
	// The chosen stage, by name - which is the manifest's name for its level,
	// and so the key its definition sits under. Empty until the stage-select
	// screen sets one.
	void set_stage(const std::string& stage);
	const std::string& stage() const;
	labrador::ScreenLayout screen_layout() const;
	void set_screen_layout(labrador::ScreenLayout screen_layout);

	void set_player_setting(int player_index, MenuPlayerSettings player_settings);
	void set_player_team(int player_index, PlayerTeam team);
	PlayerTeam player_team(int player_index) const;
	void set_player_weapon(int player_index, WeaponType weapon);
	WeaponType player_weapon(int player_index) const;
	void set_player_num(int player_index, int player_num);

	// Bind the roster once, when the player count is chosen. The device that
	// made that choice is player one; the other connected slots fill the rest.
	void assign_input_slots(int initiating_slot,
		const std::vector<int>& available_slots);
	int player_index_for_slot(int slot) const;

private:
	std::vector<MenuPlayerSettings> player_settings_;
	int player_count_ = 0;
	std::string stage_;
	labrador::ScreenLayout screen_layout_ = labrador::ScreenLayout::one_player;
};
