#pragma once

#include "game/debug/bot.h"
#include "game/objects/seat_input_filter.h"
#include <map>

class GameTuning;

// Gives the seats a bot is assigned to that bot's input, every logic step;
// other seats keep what their device gave them. Made afresh by every level
// build, from the director's assignments and the match seed. Debug-only.
class BotDriver final : public SeatInputFilter
{
public:
	BotDriver(const GameTuning* tuning, std::uint64_t seed);

	// `seat` is 0-based; `behaviour` one of Bot::behaviours().
	void assign(int seat, const std::string& behaviour);
	bool drives(int seat) const;
	const std::map<int, std::string>& assignments() const { return this->behaviours_; }

	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;

	// What a bot on `seat` sees: the level's public player state.
	// Tool observations are opt-in: legacy bots keep their original seam.
	static BotSeatView view_of(const Level& level, int seat, int step, const GameTuning& tuning,
		bool observe_tool = false);

private:
	const GameTuning* tuning_ = nullptr;
	std::uint64_t seed_ = 0;
	int step_ = 0;
	std::map<int, std::string> behaviours_;
	std::map<int, std::unique_ptr<Bot>> bots_;
};
