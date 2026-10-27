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
