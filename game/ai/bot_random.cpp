#include "game/pch.h"
#include "game/ai/bot_random.h"

BotRandom::BotRandom(std::uint64_t seed) : state_(seed)
{
}

std::uint64_t BotRandom::next()
{
	std::uint64_t z = (this->state_ += 0x9E3779B97F4A7C15ull);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	return z ^ (z >> 31);
}

float BotRandom::unit()
{
	// 24 bits: every value is exact in a float.
	return static_cast<float>(this->next() >> 40) / 16777216.0f;
}

int BotRandom::between(int low, int high)
{
	const std::uint64_t span = static_cast<std::uint64_t>(high - low) + 1;
	return low + static_cast<int>(this->next() % span);
}

float BotRandom::between(float low, float high)
{
	return low + (high - low) * this->unit();
}

std::uint64_t BotRandom::seat_seed(std::uint64_t seed, int seat)
{
	return seed * 0x9E3779B97F4A7C15ull + static_cast<std::uint64_t>(seat) + 1;
}
