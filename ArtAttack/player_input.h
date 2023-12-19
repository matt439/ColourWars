#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include "connection_state.h"
#include "direction_lock.h"
#include "MattMath.h"

struct player_input
{
	float x_movement = 0.0f;
	bool jump_pressed = false;
	bool jump_held = false;
	//bool prev_requesting_jump = false;
	//MattMath::Vector2F shoot_direction = { 0.0f, 0.0f };
	float shoot_angle = 0.0f;
	MattMath::Vector2F shoot_direction = { 0.0f, 0.0f };
	bool shoot_direction_requested = false;
	//direction_lock shoot_direction_lock = direction_lock::UNLOCKED;
	bool primary_shoot = false;
	bool secondary_shoot = false;
	//bool raw_shooting = false;
	bool toggle_debug = false;
	bool toggle_pause_menu = false;
	connection_state connection = connection_state::DISCONNECTED;

	MattMath::Vector2F left_analog_stick = { 0.0f, 0.0f };
	MattMath::Vector2F right_analog_stick = { 0.0f, 0.0f };
};

#endif // !PLAYER_INPUT_H
