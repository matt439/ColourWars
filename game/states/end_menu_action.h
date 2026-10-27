#pragma once

// Returned to the match when the end menu closes.
enum class EndMenuAction
{
	change_teams,
	change_weapons,
	change_level,
	restart,
	exit,
	// The match plays the round's instant replay, then opens the menu again.
	watch_replay,
};
