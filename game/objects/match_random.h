#pragma once

#include "game/ai/bot_random.h"

#include <cstdint>

// One round's random numbers (HYG#39), made from the seed its RoundTape keeps.
// Each use draws from its own stream, so whether one draws at all (a Random
// palette, shipping bots, an intro's persona) never moves another. Plain
// splitmix64 arithmetic: the same seed builds the same match on any standard
// library, and a build never reads or moves the global rand().
class MatchRandom
{
public:
	explicit MatchRandom(unsigned int seed = 0);

	// The round seed every stream is made from.
	unsigned int seed() const;
	// The Random team palette's draw.
	BotRandom palette() const;
	// The intro monitor's persona.
	BotRandom persona() const;
	// Shipping bots get the seed itself; BotRandom::seat_seed splits it per
	// seat, the same way the balance runner seeds its bots.
	std::uint64_t bot_seed() const;

private:
	enum class Stream : std::uint32_t { palette = 1, persona = 2 };
	BotRandom stream(Stream which) const;

	unsigned int seed_ = 0;
};
