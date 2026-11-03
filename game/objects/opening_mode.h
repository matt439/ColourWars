#pragma once

// How a match begins (docs/backstory.md, "Opening"): the whole intro; a quick
// rematch's, from the colours being tried; or straight to the split-screen
// countdown, as tests, replays and a skipped intro do.
enum class OpeningMode
{
	full,
	quick,
	off,
};

// Which opening a GameLevel's round gets (C6): its first match has the whole
// intro; every later build of the same screen (Rematch, Next Round, Replay
// Draw, Play Series Again, Pause Restart) starts with the colours being tried.
// Solo has no person at either end, so it is not a real match and gets none.
constexpr OpeningMode round_opening(bool solo, int builds)
{
	return solo ? OpeningMode::off : (builds == 0 ? OpeningMode::full : OpeningMode::quick);
}
