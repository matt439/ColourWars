#pragma once

#include "game/ai/match_bot.h"
#include "game/objects/seat_input_filter.h"
#include "game/states/menu_level_settings.h"
#include <memory>

class Player;

// Release-capable participant filter, applied after physical input mapping and
// before Debug record/play. Only explicit bot seats are replaced.
class MatchBotDriver final : public SeatInputFilter
{
public:
	MatchBotDriver(const GameTuning* tuning, const MenuLevelSettings& settings,
		std::uint64_t seed);
	bool drives(int participant) const;
	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;

private:
	const GameTuning* tuning_;
	std::vector<std::unique_ptr<MatchBot>> bots_;
	std::vector<BotDifficulty> difficulties_;
	static float engagement_distance(const Player& player, const GameTuning& tuning);
};
