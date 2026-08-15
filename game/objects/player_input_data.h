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
	bool toggle_debug = false;
	bool toggle_pause_menu = false;

	// False means the slot this entry belongs to was empty when it was filled,
	// and every field above is then neutral rather than stale. Which is not the
	// same question as "is this the pad that player has been using": a pad slot
	// is not stable across a replug, and binding a player to one for the length
	// of a match is a decision this game has not made yet. It is welded today -
	// Level::apply_player_inputs indexes by player_num() - and a controller
	// that comes back in a different slot silently drives a different
	// character. engine/input/gamepads.h reports the connect and disconnect
	// edges that a real binding would be built on.
	bool connected = false;

	mattmath::Vector2F left_analog_stick = { 0.0f, 0.0f };
	mattmath::Vector2F right_analog_stick = { 0.0f, 0.0f };
};
