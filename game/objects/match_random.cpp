#include "game/pch.h"
#include "game/objects/match_random.h"

MatchRandom::MatchRandom(unsigned int seed) : seed_(seed)
{
}

unsigned int MatchRandom::seed() const
{
	return this->seed_;
}

BotRandom MatchRandom::palette() const
{
	return this->stream(Stream::palette);
}

BotRandom MatchRandom::persona() const
{
	return this->stream(Stream::persona);
}

std::uint64_t MatchRandom::bot_seed() const
{
	return this->seed_;
}

// A stream's state is the first splitmix64 output of (tag << 32 | seed), so
// two streams of one seed, or one stream of two seeds, never share a start.
BotRandom MatchRandom::stream(Stream which) const
{
	BotRandom start((static_cast<std::uint64_t>(which) << 32) | this->seed_);
	return BotRandom(start.next());
}
