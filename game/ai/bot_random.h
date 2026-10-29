#pragma once

#include <cstdint>

// A bot's own random numbers: splitmix64, seeded from the match seed and the
// seat, so a bot never touches rand() and two bots never share a sequence.
// The shipping MatchBot and the Debug scripted bots both draw from this.
class BotRandom
{
public:
	explicit BotRandom(std::uint64_t seed = 0);

	std::uint64_t next();
	// [0, 1).
	float unit();
	// [low, high], inclusive.
	int between(int low, int high);
	float between(float low, float high);

	// The seed for the bot on `seat` (0-based) in a match seeded `seed`.
	static std::uint64_t seat_seed(std::uint64_t seed, int seat);

private:
	std::uint64_t state_ = 0;
};
