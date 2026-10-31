#pragma once

#include "game/ai/bot_world.h"
#include "game/ai/match_bot.h"
#include "game/match_setup.h"
#include <memory>
#include <optional>
#include <vector>

// The shipping bots' seats, driven after physical input mapping and before
// Debug record/play. Only explicit bot seats are replaced. It sees the match
// only through BotWorld, so game/ai never names Level.
class MatchBotDriver final
{
public:
	MatchBotDriver(const GameTuning* tuning, const MatchSetup& settings,
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
	// `seats` has one entry per participant, in roster order.
	void drive(std::vector<PlayerInputData>& seats, const BotWorld& world);

private:
	const GameTuning* tuning_;
	std::vector<std::unique_ptr<MatchBot>> bots_;
	std::vector<BotDifficulty> difficulties_;
	std::vector<PlayerTeam> gate_permissions_;
	static float engagement_distance(const BotBody& body, const GameTuning& tuning);
};
