#pragma once

// Returned once through the state stack when the pause menu closes.
enum class PauseMenuAction
{
	resume,
	restart,
	quit,
	// Untimed practice only: the page has written the chosen tool into the
	// match settings; rebuild with it, keeping the player's place.
	change_tool,
};
