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
