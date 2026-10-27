#pragma once

#include "game/input_bindings.h"
#include "game/play_settings.h"
#include "game/save_section.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// A named player profile (Smash's name tag): what one person carries from pad
// to pad. A pad that wears a profile reads these instead of its own slot's
// stick, rumble, comfort and button settings; Boost is the seat's starting
// handicap whenever that pad joins. The keyboard seat and bots wear none.
struct PlayerProfile
{
	std::string name;
	StickSettings sticks;
	bool rumble = true;
	SeatComfort comfort;
	PadProfile buttons;
	// handicaps::valid; 0 is none.
	int boost = 0;

	bool operator==(const PlayerProfile& other) const
	{
		return this->name == other.name && this->sticks.movement_deadzone == other.sticks.movement_deadzone &&
			this->sticks.aim_deadzone == other.sticks.aim_deadzone && this->rumble == other.rumble &&
			this->comfort == other.comfort && this->buttons == other.buttons && this->boost == other.boost;
	}
};

namespace player_profiles
{
	constexpr std::size_t max_count = 8;
	constexpr std::size_t max_name_length = 10;

	// The characters a name may hold, in the order the name editor cycles
	// them: space first, then A-Z, then 0-9. Names are stored as written.
	std::string_view name_characters();
	bool name_character_supported(char value);
	// Trims surrounding spaces. Throws invalid_argument for an empty result,
	// more than max_name_length characters or an unsupported character.
	std::string normalized_name(std::string_view name);
}

// Every profile, in the order they were made. Names are unique.
struct PlayerProfiles
{
	std::vector<PlayerProfile> list;

	// At most max_count; each name normalized and unique, each pad profile
	// valid and each Boost in handicaps::valid. Throws invalid_argument.
	void validate() const;
	// -1 when no profile has this name.
	int find(std::string_view name) const;
	// The first of "PLAYER 1" through "PLAYER 8" no profile uses.
	std::string next_free_name() const;
	bool full() const { return this->list.size() >= player_profiles::max_count; }
	bool operator==(const PlayerProfiles&) const = default;
};

// "profiles": [ { "name": "ALEX", "movement_deadzone": -1, "aim_deadzone": -1,
// "rumble": true, "view_shake": "shared", "aim_marker": "shared",
// "reduce_effects": "shared", "buttons": { "jump": ["a", "lt"], ... },
// "boost": 0 } ]. Each field recovers to its default independently; an entry
// without a usable name, or one whose name repeats an earlier entry, is
// dropped, and entries past max_count are ignored.
class PlayerProfilesSection final : public SaveSection
{
public:
	const char* key() const override { return "profiles"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
