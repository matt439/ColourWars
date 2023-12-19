#ifndef PLAYERINPUT_H
#define PLAYERINPUT_H

#include <vector>
#include "connection_state.h"
#include <cmath>
#include "player_input.h"
#include "MattMath.h"

namespace player_input_consts
{
	constexpr float TRIGGER_SHOOT_THRESHOLD = 0.6f;
	constexpr float TRIGGER_JUMP_THRESHOLD = 0.6f;
	constexpr float STICK_DEADZONE = 0.5f;
}

struct raw_player_input
{
	MattMath::Vector2F left_analog_stick = { 0.0f, 0.0f };
	MattMath::Vector2F right_analog_stick = { 0.0f, 0.0f };
	bool jump_button = false;
	float jump_trigger = 0.0f;
	float primary_shoot = 0.0f;
	bool secondary_shoot = false;
	bool show_debug = false;
	bool pause_menu = false;
	bool connected = false;
};

class PlayerInput
{
private:
	raw_player_input _prev_inputs[4];
	raw_player_input get_raw_input(int gamepad_num);
	player_input calculate_player_input(const raw_player_input& current,
		const raw_player_input& previous);
	DirectX::GamePad* _gamepad = nullptr;
public:
	PlayerInput(DirectX::GamePad* gamepad);
	std::vector<player_input> update_and_get_player_inputs();
};

#endif // !PLAYER_INPUT_H
