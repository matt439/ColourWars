#pragma once

#include "game/objects/player_input_data.h"

// What one device slot says in one frame (PlayerInput::player_inputs): the
// command its seat would play, plus the signals only the shell reads. GameLevel
// takes pause from these, and SeatCommandBuilder turns a participant's skip
// into the level's intro-skip request; neither reaches a seat, a bot, a round
// tape or a Debug replay.
struct SlotInput : PlayerInputData
{
	// Start or Escape released: opens the pause menu, unless a latch eats it.
	bool toggle_pause_menu = false;
	// Start or Escape down: lets a press begun over the intro be told apart
	// from a fresh one when it is released (latch_intro_pause).
	bool pause_held = false;
	// A, B or Start released (Space, Escape or Enter): skips the intro.
	bool skip_intro = false;
};
