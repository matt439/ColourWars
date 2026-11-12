#pragma once

#include <string>

// `replay.timelapse` (docs/debug-console.md, "Time-lapse"): the arithmetic the
// command and its acceptance check share. Debug-only callers.
namespace timelapse
{
	// Taped steps a frame: the default, and the accepted range.
	constexpr int default_speed = 16;
	constexpr int max_speed = 64;
	// Stills a run may save.
	constexpr int max_stills = 32;

	// The taped step from which still `index` (0-based) of `stills` is due,
	// for a tape of `steps`: evenly spaced, the last on the final step.
	inline int still_step(int index, int stills, int steps)
	{
		if (stills <= 0 || steps <= 0) { return 0; }
		return static_cast<int>((static_cast<long long>(index + 1) * steps + stills - 1) / stills);
	}

	// "timelapse-<stamp>-07.png": `stamp` names the run, `index` is 0-based.
	inline std::string still_name(const std::string& stamp, int index)
	{
		const int number = index + 1;
		return "timelapse-" + stamp + "-" + (number < 10 ? "0" : "") + std::to_string(number) + ".png";
	}
}
