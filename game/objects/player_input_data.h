#pragma once

#include "engine/input/connection_state.h"
#include "engine/math/matt_math.h"

struct PlayerInputData
{
	float x_movement = 0.0f;
	bool jump_pressed = false;
	bool jump_held = false;
	float shoot_angle = 0.0f;
	mattmath::Vector2F shoot_direction = { 0.0f, 0.0f };
	bool shoot_direction_requested = false;
	bool primary_shoot = false;
	bool secondary_shoot = false;
	bool toggle_debug = false;
	bool toggle_pause_menu = false;
	artattack::ConnectionState connection = artattack::ConnectionState::disconnected;

	mattmath::Vector2F left_analog_stick = { 0.0f, 0.0f };
	mattmath::Vector2F right_analog_stick = { 0.0f, 0.0f };
};
