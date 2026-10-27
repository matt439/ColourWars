#pragma once

#include <optional>
#include <string>
#include <vector>
#include "engine/render/screen_layout.h"
#include "game/objects/player_team.h"
#include "game/objects/stage.h"
#include "game/objects/weapon_type.h"
#include "game/player_control.h"
#include "game/save_section.h"

class MenuLevelSettings;

// A seat's resolved choices, without a device claim or confirmation state.
struct LastMatchPlayer
{
	PlayerTeam team = PlayerTeam::none;
	WeaponType weapon = WeaponType::sprayer;
	PlayerController controller = PlayerController::human;
	// Only bots have a difficulty; human entries use the canonical Normal value.
	BotDifficulty difficulty = BotDifficulty::normal;
	// handicaps::valid, humans and bots alike; 0 is none.
	int handicap = 0;
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
	// Returns fresh, unclaimed human seats and retains device-less bot seats.
	// A removed stage uses the first registered
	// stage, including arenas excluded from Random. An empty list is invalid.
	MenuLevelSettings restore(const StageList& stages) const;
	// Repairs recoverable saved fields without randomness. Counts outside 1-4
	// cannot form a setup and throw invalid_argument. An empty stage is left for
	// restore(), which has the current catalogue needed to choose a fallback.
	LastMatchSetup normalised() const;
	// For new captures and writes: require resolved choices, not field recovery.
	void validate() const;
};

// "last_match": { "players": [ { "team": "a", "weapon": "sprayer",
// "controller": "human", "difficulty": "normal", "handicap": 1 } ],
// "stage": "...", "layout": "one_player", "best_of": 1, "duration": ... }.
// Default duration is represented by omitting duration. Invalid whole sections
// or counts reset to absent; recoverable fields are normalised independently.
// Legacy seats without controller/difficulty are human/Normal. Handicap is
// written only when a seat has one; absent or invalid is none. P1 is human,
// and the layout follows the number of humans rather than all participants.
class LastMatchSetupSection final : public SaveSection
{
public:
	const char* key() const override { return "last_match"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
