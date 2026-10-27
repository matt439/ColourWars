#pragma once

#include <array>
#include <string>
#include <optional>
#include <vector>
#include "game/objects/weapon_type.h"
#include "game/states/menu_player_settings.h"
#include "engine/render/screen_layout.h"
#include "game/input_slots.h"

struct MenuText;

// The roster is empty until a player count is chosen, then contains 1-4 seats.
// Human seats are unclaimed (-1) or hold a unique valid device slot. Bots have
// no device and carry their own difficulty. P1 is always the human setup host.
// Only roster operations mutate assignments; snapshots cannot alter the roster.
class MenuLevelSettings
{
public:
	MenuLevelSettings() = default;
	int best_of() const;
	void set_best_of(int best_of);
	std::optional<float> duration() const;
	// No override uses match.duration from tuning, including live Debug tuning.
	void set_duration(std::optional<float> seconds);
	const std::vector<MenuPlayerSettings>& player_settings() const;
	// Preserves retained seats and defaults newly added seats to unclaimed.
	// Rejects counts outside 1-4 without changing the roster or its count.
	void set_player_count(int player_count);
	int player_count() const;
	int human_count() const;
	int bot_count() const;
	// Replacing a human with a bot releases its device. P1 cannot become a bot.
	// Updating an existing bot changes only difficulty; team/weapon stay intact.
	void set_player_bot(int player_index, BotDifficulty difficulty);
	// Replacing a bot leaves an unclaimed human. Existing human claims stay put.
	void set_player_human(int player_index);
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
	// human seat. A disconnected device retains its seat until setup is cancelled.
	// Beginning claims resets human devices while retaining bot choices.
	void begin_input_claims(int initiating_slot);
	bool claim_input_slot(int slot);
	bool release_input_slot(int slot);
	// Bots occupy their seats without claiming a device; all humans must claim.
	bool input_roster_complete() const;
	int player_index_for_slot(int slot) const;

	// A seat's input slot (MenuPlayerSettings::num) is its identity for the
	// match: paint, scores and preferences are keyed on it. The pad it reads
	// is device_slot(num), the same slot unless a controller that came back
	// in another slot reclaimed the seat (ROAD#4). Setup never moves one;
	// every claim operation above puts each seat back on its own slot.
	int device_slot(int slot) const;
	// device_slot for every pad slot, for readers indexed by input slot.
	std::array<int, input_slots::pad_count> pad_devices() const;
	// The seat whose input arrives on this device, or -1. A seat that moved
	// no longer answers for the slot it left.
	int player_index_for_device(int device) const;
	// Move a human pad seat onto a pad no seat reads. Returns false and
	// changes nothing otherwise; moving back to its own slot is allowed.
	bool move_to_device(int slot, int device);
	// Stable participant number, with a human device or explicit bot difficulty.
	std::wstring player_label(int player_index, const MenuText& text) const;

private:
	void reset_devices();

	std::vector<MenuPlayerSettings> player_settings_;
	// Per pad input slot, the pad that now drives it; -1 is the slot itself.
	std::array<int, input_slots::pad_count> devices_ = { -1, -1, -1, -1 };
	int best_of_ = 1;
	std::optional<float> duration_;
	std::string stage_;
	labrador::ScreenLayout screen_layout_ = labrador::ScreenLayout::one_player;
};
