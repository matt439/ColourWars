#pragma once

// What the pause menu was closed with. It is the result the state stack carries
// back down to the match (state_context.h), so there is no `none`: the match is
// told once, when the menu pops, instead of reading a heap-allocated enum every
// frame to see whether anything has happened yet.
enum class PauseMenuAction
{
	resume,
	restart,
	quit,
};
