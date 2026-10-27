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
	// Like restart, on the stage just chosen by hand on Stage Select between
	// series rounds; the series stage rule does not move it (ROAD#20).
	play_chosen_stage,
};
