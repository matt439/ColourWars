#include "pch.h"
#include "PlayerInput.h"

using namespace DirectX;
using namespace MattMath;
using namespace player_input_consts;

PlayerInput::PlayerInput(GamePad* gamepad) :
    _gamepad(gamepad)
{
    for (auto& _prev_input : this->_prev_inputs)
    {
	    _prev_input = RawPlayerInput();
    }
}

RawPlayerInput PlayerInput::get_raw_input(int gamepad_num) const
{
	auto result = RawPlayerInput();
    auto pad = this->_gamepad->GetState(gamepad_num, GamePad::DEAD_ZONE_CIRCULAR);
    if (pad.IsConnected())
    {
        result.left_analog_stick = Vector2F(pad.thumbSticks.leftX,
            -pad.thumbSticks.leftY);
        result.right_analog_stick = Vector2F(pad.thumbSticks.rightX,
			-pad.thumbSticks.rightY);
        result.jump_button = pad.buttons.a;
        result.jump_trigger = pad.triggers.left;
        result.primary_shoot = pad.triggers.right;
        result.secondary_shoot = pad.buttons.rightShoulder;
        result.show_debug = pad.buttons.back;
        result.pause_menu = pad.buttons.start;
        result.connected = true;
    }
    return result;
}

PlayerInputData PlayerInput::calculate_player_input(
    const RawPlayerInput& current, const RawPlayerInput& previous)
{
	auto result = PlayerInputData();

    result.jump_held = current.jump_button || current.jump_trigger > TRIGGER_JUMP_THRESHOLD;

    bool jump_button_pressed = current.jump_button && !previous.jump_button;
    bool jump_trigger_pressed = current.jump_trigger > TRIGGER_JUMP_THRESHOLD &&
        previous.jump_trigger <= TRIGGER_JUMP_THRESHOLD;
    result.jump_pressed = jump_button_pressed || jump_trigger_pressed;
    
    result.secondary_shoot = current.secondary_shoot;

    // pause menu is on key press
    bool pause_menu = !current.pause_menu && previous.pause_menu;
    if (pause_menu)
	{
		result.toggle_pause_menu = true;
	}
	else
	{
		result.toggle_pause_menu = false;
	}

    // show debug is on key press
    bool show_debug = !current.show_debug && previous.show_debug;
    if (show_debug)
	{
		result.toggle_debug = true;
	}
	else
	{
		result.toggle_debug = false;
	}

    // primary shoot is analog trigger
    // so need to check if it has gone past threshold
    bool primary_shoot = current.primary_shoot > TRIGGER_SHOOT_THRESHOLD;
    if (primary_shoot)
	{
        result.primary_shoot = true;
	}
	else
{
        result.primary_shoot = false;
	}

    if (std::fabs(current.left_analog_stick.x) > STICK_DEADZONE)
    {
		result.x_movement = current.left_analog_stick.x;
	}
    else
    {
        result.x_movement = 0.0f;
    }


    Vector2F direction = current.right_analog_stick;
    if (direction.length() > STICK_DEADZONE)
	{
		direction.normalize();
        result.shoot_angle = direction.angle();
        result.shoot_direction = direction;
        result.shoot_direction_requested = true;
	}
	else
	{
        result.shoot_direction = Vector2F::ZERO;
        result.shoot_angle = 0.0f;
        result.shoot_direction_requested = false;
	}

    result.connection = connection_state::CONNECTED;

    result.left_analog_stick = current.left_analog_stick;
    result.right_analog_stick = current.right_analog_stick;

    return result;
}

std::vector<PlayerInputData> PlayerInput::update_and_get_player_inputs()
{
    RawPlayerInput current[4];
    for (int i = 0; i < 4; i++)
    {
        current[i] = this->get_raw_input(i);
    }
    std::vector<PlayerInputData> result;
    for (int i = 0; i < 4; i++)
    {
        if (current[i].connected)
        {
            PlayerInputData input = calculate_player_input(current[i],
                this->_prev_inputs[i]);
            result.push_back(input);
        }
        this->_prev_inputs[i] = current[i];
    }
    return result;
}