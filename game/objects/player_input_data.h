#pragma once

#include "engine/math/vector2f.h"

// One seat's command for one logic step: what its player does. Level::update
// takes one per participant, in roster order, already mapped from its device
// slot with keyboard/mouse aim resolved (SeatCommandBuilder); bots, round tapes
// and Debug replays write and record this and nothing else. What a device slot
// also says to the shell - pause, the intro skip - is SlotInput's.
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

	// The seat's own F1 / Back overlay: Player flips it on the steps it runs.
	bool toggle_debug = false;

	// False for a disconnected slot, whose actions are neutral. Players remain
	// bound to device slots for the match; a pad back in a different slot
	// drives nobody until it reclaims a missing seat with Start (ROAD#4).
	bool connected = false;

	mattmath::Vector2F left_analog_stick = { 0.0f, 0.0f };
	mattmath::Vector2F right_analog_stick = { 0.0f, 0.0f };
};
