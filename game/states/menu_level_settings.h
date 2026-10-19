#pragma once

#include <string>
#include <optional>
#include <vector>
#include "game/objects/weapon_type.h"
#include "game/states/menu_player_settings.h"
#include "engine/render/screen_layout.h"

struct MenuText;

// The roster is empty until a player count is chosen, then contains 1-4 seats.
// Each seat is unclaimed (-1) or holds a unique valid device slot. Only the
// claim operations mutate device assignments; snapshots cannot alter the roster.
class MenuLevelSettings
{
public:
	MenuLevelSettings() = default;
	int best_of() const;
	void set_best_of(int best_of);
	std::optional<float> duration() const;
	// No override uses match.duration from tuning, including live Debug tuning.
	void set_duration(std::optional<float> seconds);
	std::vector<MenuPlayerSettings> player_settings() const;
	// Preserves retained seats and defaults newly added seats to unclaimed.
	// Rejects counts outside 1-4 without changing the roster or its count.
	void set_player_count(int player_count);
	int player_count() const;
	// Manifest key for the chosen level; empty until stage selection.
	void set_stage(const std::string& stage);
	const std::string& stage() const;
	labrador::ScreenLayout screen_layout() const;
	void set_screen_layout(labrador::ScreenLayout screen_layout);

	// Indexed access rejects seats outside the selected roster.
	void set_player_team(int player_index, PlayerTeam team);
	PlayerTeam player_team(int player_index) const;
	// A complete multiplayer roster gets balanced random teams. Changes only
	// team assignments; solo, empty and partly joined rosters are left alone.
	bool shuffle_teams();
	void set_player_weapon(int player_index, WeaponType weapon);
	WeaponType player_weapon(int player_index) const;

	// Player one chooses the count; everyone else explicitly claims an empty
	// seat. A disconnected device retains its seat until setup is cancelled.
	void begin_input_claims(int initiating_slot);
	bool claim_input_slot(int slot);
	bool release_input_slot(int slot);
	bool input_roster_complete() const;
	int player_index_for_slot(int slot) const;
	std::wstring player_label(int player_index, const MenuText& text) const;

private:
	std::vector<MenuPlayerSettings> player_settings_;
	int best_of_ = 1;
	std::optional<float> duration_;
	std::string stage_;
	labrador::ScreenLayout screen_layout_ = labrador::ScreenLayout::one_player;
};
