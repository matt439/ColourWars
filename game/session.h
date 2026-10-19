#pragma once

#include "game/objects/level_end_info.h"

struct MenuEndText;

// Win ledger for a fixed-roster series. Draws award no win, so best-of counts
// decisive rounds. Restarting an unfinished round leaves the ledger unchanged;
// finish_round is idempotent until next_round.
class Session
{
public:
	explicit Session(int best_of = 1);
	void finish_round(const LevelEndInfo& result);
	void next_round();
	bool is_series() const;
	bool complete() const;
	int round() const;
	int team_a_wins() const;
	int team_b_wins() const;
	std::wstring score_string() const;
	std::wstring result_string() const;
	std::wstring continue_string(const MenuEndText& text) const;

private:
	int best_of_ = 1;
	int round_ = 1;
	int team_a_wins_ = 0;
	int team_b_wins_ = 0;
	bool round_finished_ = false;
	PlayerTeam round_winner_ = PlayerTeam::none;
};
