#pragma once

// The main-menu frame's result: only the final normal Stage confirmation may
// remember a setup. Developer requests use the same frame without that write.
enum class MatchStart
{
	menu,
	developer
};
