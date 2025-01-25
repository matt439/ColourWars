#ifndef PLAYERINPUT_H
#define PLAYERINPUT_H

#include <vector>
#include "PlayerInputData.h"
#include "MattMath.h"

namespace player_input_consts
{
	constexpr float TRIGGER_SHOOT_THRESHOLD = 0.6f;
	constexpr float TRIGGER_JUMP_THRESHOLD = 0.6f;
	constexpr float STICK_DEADZONE = 0.5f;
}

struct RawPlayerInput
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
public:
	explicit PlayerInput(DirectX::GamePad* gamepad);
	std::vector<PlayerInputData> update_and_get_player_inputs();
private:
	RawPlayerInput _prev_inputs[4];
	RawPlayerInput get_raw_input(int gamepad_num) const;
	static PlayerInputData calculate_player_input(const RawPlayerInput& current,
	                                              const RawPlayerInput& previous);
	DirectX::GamePad* _gamepad = nullptr;
};

#endif // !PLAYER_INPUT_H
