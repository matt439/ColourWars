#pragma once

#include <string>
#include "game/objects/player_team.h"
#include "game/objects/stage.h"

class MenuLevelSettings;
class Session;

// Which stage a series' next round plays (ROAD#20). Chosen on the Mode page
// beside the series length and remembered with the setup. A single match has
// no next round, so the rule does nothing there.
enum class SeriesStageRule
{
	// Every round on the stage the series started on: the original behaviour.
	same,
	// After a decisive round, the next stage of the Random pool, in manifest order.
	rotate,
	// After a decisive round, the losing team chooses on Stage Select.
	losers_pick
};

// The save's spelling: "same", "rotate", "losers_pick".
const char* series_stage_rule_key(SeriesStageRule rule);
// False for an unknown spelling, leaving `rule` unchanged.
bool read_series_stage_rule(const std::string& key, SeriesStageRule& rule);

// The Random pool stage after `current` in manifest order, wrapping. Stages
// left out of Random are never rotated into; from one of them the rotation
// goes on to the next pool stage after it. An empty pool keeps `current`.
std::string rotated_stage(const StageList& stages, const std::string& current);

// The team that opens Stage Select before the next round: under Loser's pick,
// the loser of the finished round when that team has a human seat. None after
// a draw, under another rule, outside a series, or for a bot-only loser.
PlayerTeam series_stage_picker(const MenuLevelSettings& settings, const Session& session);

// The stage the next round plays. A stage chosen by hand between rounds
// (Change Level) is already in `settings` and always wins. Otherwise a draw
// replays its stage under every rule (the End menu calls it Replay Draw), Same
// keeps the stage, and Rotate moves on. Loser's pick moves on the same way
// only when the loser is all bots and so cannot pick; a human loser picks on
// Stage Select, which arrives here already chosen by hand.
std::string series_next_stage(const MenuLevelSettings& settings, const StageList& stages,
	const Session& session, bool chosen_by_hand);
