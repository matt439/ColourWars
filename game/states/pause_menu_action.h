#pragma once

// Returned once through the state stack when the pause menu closes.
enum class PauseMenuAction
{
	resume,
	restart,
	quit,
};
