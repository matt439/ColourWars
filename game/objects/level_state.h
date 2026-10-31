#pragma once

// Where a match is: Level steps it, and the opening it owns reads it.
enum class LevelState
{
	start_countdown,
	active,
	zoom_out,
	overview,
	finished,
	// Before the countdown. Last, so the values above keep their numbers.
	opening,
};
