#pragma once

#include <array>
#include <optional>
#include <stdexcept>
#include <string_view>

// Who supplies a participant's gameplay input. Bots have no physical device
// slot; an unclaimed human is still human (including the Debug dummy seats).
enum class PlayerController
{
	human,
	bot
};

enum class BotDifficulty
{
	easy,
	normal,
	hard
};

// Every difficulty, easiest first, which is also the index of its entry in
// BotTuning::difficulties.
constexpr std::array<BotDifficulty, 3> bot_difficulties{
	BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };

constexpr bool valid_bot_difficulty(BotDifficulty difficulty)
{
	for (const BotDifficulty known : bot_difficulties)
	{
		if (known == difficulty) { return true; }
	}
	return false;
}

// The one spelling the save, the Debug replay file and plan, tuning.json and
// the console use: "easy", "normal", "hard". Throws std::invalid_argument for
// a value outside the enum.
inline const char* bot_difficulty_key(BotDifficulty difficulty)
{
	switch (difficulty)
	{
	case BotDifficulty::easy: return "easy";
	case BotDifficulty::normal: return "normal";
	case BotDifficulty::hard: return "hard";
	}
	throw std::invalid_argument("Unknown bot difficulty");
}

// Empty for an unknown spelling.
inline std::optional<BotDifficulty> bot_difficulty_from_key(std::string_view key)
{
	for (const BotDifficulty difficulty : bot_difficulties)
	{
		if (key == bot_difficulty_key(difficulty)) { return difficulty; }
	}
	return std::nullopt;
}

// A seat's handicap level, chosen on the Tools page for humans and bots alike.
// Level 0 is none; each higher level is a bigger boost, whose amounts are
// tuning.json "handicaps". Score is still painted surface at every level.
namespace handicaps
{
	constexpr int count = 3;
	constexpr bool valid(int level) { return level >= 0 && level < count; }
}
