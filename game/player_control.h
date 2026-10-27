#pragma once

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

// A seat's handicap level, chosen on the Tools page for humans and bots alike.
// Level 0 is none; each higher level is a bigger boost, whose amounts are
// tuning.json "handicaps". Score is still painted surface at every level.
namespace handicaps
{
	constexpr int count = 3;
	constexpr bool valid(int level) { return level >= 0 && level < count; }
}
