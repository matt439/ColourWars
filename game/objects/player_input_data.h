#pragma once

#include "engine/math/vector2f.h"

struct PlayerInputData
{
	float x_movement = 0.0f;
	bool jump_pressed = false;
	bool jump_held = false;
	float shoot_angle = 0.0f;
	mattmath::Vector2F shoot_direction = { 0.0f, 0.0f };
	bool shoot_direction_requested = false;
	bool primary_shoot = false;

	// Held request to drop through a platform, independent of input device.
	bool drop_through = false;

	bool toggle_debug = false;
	bool toggle_pause_menu = false;
	// Start or Escape down: lets a press begun over the intro be told apart
	// from a fresh one when it is released (latch_intro_pause).
	bool pause_held = false;
	// A, B or Start released (Space, Escape or Enter): skips the intro.
	bool skip_intro = false;

	// False for a disconnected slot, whose actions are neutral. Players remain
	// bound to device slots for the match; a reconnected pad in a different slot
	// controls that slot's player, if any.
	bool connected = false;

	mattmath::Vector2F left_analog_stick = { 0.0f, 0.0f };
	mattmath::Vector2F right_analog_stick = { 0.0f, 0.0f };
};
