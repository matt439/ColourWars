#pragma once

#include "game/ai/match_bot.h"
#include "game/objects/seat_input_filter.h"
#include "game/states/menu_level_settings.h"
#include <memory>
#include <optional>
#include <vector>

class Player;

// Release-capable participant filter, applied after physical input mapping and
// before Debug record/play. Only explicit bot seats are replaced.
class MatchBotDriver final : public SeatInputFilter
{
public:
	MatchBotDriver(const GameTuning* tuning, const MenuLevelSettings& settings,
		std::uint64_t seed);
	// One entry per participant, a difficulty for each bot and nullopt for a
	// human. The roster form above is this with the roster's bot seats. Only
	// a measurement uses it directly: a roster cannot make P1 a bot, and a
	// one-on-one bot match needs one.
	MatchBotDriver(const GameTuning* tuning, const std::vector<std::optional<BotDifficulty>>& seats,
		std::uint64_t seed);
	bool drives(int participant) const;
	// The seat's policy, or null for a human seat. Read-only, for measurement.
	const MatchBot* bot(int participant) const;
	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;

private:
	const GameTuning* tuning_;
	std::vector<std::unique_ptr<MatchBot>> bots_;
	std::vector<BotDifficulty> difficulties_;
	std::vector<PlayerTeam> gate_permissions_;
	static float engagement_distance(const Player& player, const GameTuning& tuning);
};
