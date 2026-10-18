#pragma once

#include <optional>
#include <string>
#include <vector>
#include "engine/render/screen_layout.h"
#include "game/objects/player_team.h"
#include "game/objects/stage.h"
#include "game/objects/weapon_type.h"
#include "game/save_section.h"

class MenuLevelSettings;

// A seat's resolved choices, without a device claim or confirmation state.
struct LastMatchPlayer
{
	PlayerTeam team = PlayerTeam::none;
	WeaponType weapon = WeaponType::sprayer;
};

// Preferences from one normally started match, independent of its progress.
// The players array is also the player count. An absent Save section, rather
// than an empty setup, represents a user who has not remembered a match yet.
struct LastMatchSetup
{
	std::vector<LastMatchPlayer> players;
	std::string stage;
	labrador::ScreenLayout layout = labrador::ScreenLayout::one_player;
	std::optional<float> duration;
	int best_of = 1;

	// Rejects unresolved or invalid choices; input claims are neither required
	// nor copied. The caller establishes that this was a normal user start.
	static LastMatchSetup capture(const MenuLevelSettings& settings);
	// Returns fresh, unclaimed seats. A removed stage uses the first registered
	// stage, including arenas excluded from Random. An empty list is invalid.
	MenuLevelSettings restore(const StageList& stages) const;
	// Repairs recoverable saved fields without randomness. Counts outside 1-4
	// cannot form a setup and throw invalid_argument. An empty stage is left for
	// restore(), which has the current catalogue needed to choose a fallback.
	LastMatchSetup normalised() const;
	// For new captures and writes: require resolved choices, not field recovery.
	void validate() const;
};

// "last_match": { "players": [ { "team": "a", "weapon": "sprayer" } ],
// "stage": "...", "layout": "one_player", "best_of": 1, "duration": ... }.
// Default duration is represented by omitting duration. Invalid whole sections
// or counts reset to absent; recoverable fields are normalised independently.
class LastMatchSetupSection final : public SaveSection
{
public:
	const char* key() const override { return "last_match"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
