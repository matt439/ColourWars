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
	int best_of() const;
	void set_best_of(int best_of);
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

	// Player one chooses the count; everyone else explicitly claims an empty
	// seat. A disconnected device retains its seat until setup is cancelled.
	void begin_input_claims(int initiating_slot);
	bool claim_input_slot(int slot);
	bool release_input_slot(int slot);
	bool input_roster_complete() const;
	int player_index_for_slot(int slot) const;
	std::wstring player_label(int player_index) const;

private:
	std::vector<MenuPlayerSettings> player_settings_;
	int player_count_ = 0;
	int best_of_ = 1;
	std::string stage_;
	labrador::ScreenLayout screen_layout_ = labrador::ScreenLayout::one_player;
};
