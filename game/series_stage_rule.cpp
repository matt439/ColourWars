#include "game/pch.h"
#include "game/series_stage_rule.h"
#include "game/session.h"
#include "game/states/menu_level_settings.h"
#include <algorithm>
#include <array>
#include <utility>

namespace
{
	constexpr std::array<std::pair<SeriesStageRule, const char*>, 3> rule_keys{ {
		{ SeriesStageRule::same, "same" },
		{ SeriesStageRule::rotate, "rotate" },
		{ SeriesStageRule::losers_pick, "losers_pick" } } };

	PlayerTeam round_loser(const Session& session)
	{
		switch (session.round_winner())
		{
		case PlayerTeam::a: return PlayerTeam::b;
		case PlayerTeam::b: return PlayerTeam::a;
		case PlayerTeam::none: break;
		}
		return PlayerTeam::none;
	}

	bool has_human(const MenuLevelSettings& settings, PlayerTeam team)
	{
		const std::vector<MenuPlayerSettings>& seats = settings.player_settings();
		return std::any_of(seats.begin(), seats.end(), [team](const MenuPlayerSettings& seat)
			{ return seat.team == team && seat.controller == PlayerController::human; });
	}
}

const char* series_stage_rule_key(SeriesStageRule rule)
{
	for (const auto& [value, key] : rule_keys)
	{
		if (value == rule) { return key; }
	}
	throw std::invalid_argument("Unknown series stage rule");
}

bool read_series_stage_rule(const std::string& key, SeriesStageRule& rule)
{
	for (const auto& [value, name] : rule_keys)
	{
		if (key == name) { rule = value; return true; }
	}
	return false;
}

std::string rotated_stage(const StageList& stages, const std::string& current)
{
	const auto found = std::find_if(stages.begin(), stages.end(),
		[&current](const Stage& stage) { return stage.name == current; });
	// An unknown stage starts the rotation from the top of the manifest.
	const size_t start = found == stages.end() ? stages.size() - 1 :
		static_cast<size_t>(found - stages.begin());
	for (size_t step = 1; step <= stages.size(); ++step)
	{
		const Stage& candidate = stages[(start + step) % stages.size()];
		if (candidate.include_in_random) { return candidate.name; }
	}
	return current;
}

PlayerTeam series_stage_picker(const MenuLevelSettings& settings, const Session& session)
{
	if (!session.is_series() || settings.series_stage_rule() != SeriesStageRule::losers_pick)
	{
		return PlayerTeam::none;
	}
	const PlayerTeam loser = round_loser(session);
	return loser != PlayerTeam::none && has_human(settings, loser) ? loser : PlayerTeam::none;
}

std::string series_next_stage(const MenuLevelSettings& settings, const StageList& stages,
	const Session& session, bool chosen_by_hand)
{
	const std::string& current = settings.stage();
	if (chosen_by_hand || !session.is_series() || session.round_winner() == PlayerTeam::none)
	{
		return current;
	}
	switch (settings.series_stage_rule())
	{
	case SeriesStageRule::same:
		return current;
	case SeriesStageRule::rotate:
		return rotated_stage(stages, current);
	case SeriesStageRule::losers_pick:
		// A human loser chose on Stage Select; one that could not, moves on.
		return series_stage_picker(settings, session) == PlayerTeam::none ?
			rotated_stage(stages, current) : current;
	}
	return current;
}
