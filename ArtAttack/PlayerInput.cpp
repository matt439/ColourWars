#include "pch.h"
#include "PlayerInput.h"

using namespace DirectX;
using namespace MattMath;
using namespace player_input_consts;

PlayerInput::PlayerInput(GamePad* gamepad) :
    _gamepad(gamepad)
{
    for (int i = 0; i < 4; i++)
    {
        this->_prev_inputs[i] = raw_player_input();
    }
}

raw_player_input PlayerInput::get_raw_input(int gamepad_num)
{
    raw_player_input result = raw_player_input();
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

player_input PlayerInput::calculate_player_input(
    const raw_player_input& current, const raw_player_input& previous)
{
    player_input result = player_input();

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


 //   Vector2F direction = current.left_analog_stick;
 //   if (std::abs(direction.x) > STICK_DEADZONE)
 //   {
 //       //direction.normalize();
 //       result.x_movement = direction.x;
 //   }
	//else
	//{
	//	result.x_movement = 0.0f;
	//}

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

    //// combined x movement and shoot direction.
    //// if left analog stick is pushed far enough
    //// then use that as the shoot direction
    //// otherwise use the shoot direction
    //// as the direction the player is facing
    //Vector2F direction = current.left_analog_stick;
    //if (abs(direction.x) > 0.7f)
    //{
    //    result.x_movement = direction.x;
    //    direction.normalize();
    //    result.shoot_direction = direction;
    //    result.shoot_direction_lock = direction_lock::UNLOCKED;
    //}
    //else if (direction.length() > 0.3f)
    //{
    //    direction.normalize();
    //    result.shoot_direction = direction;
    //    result.shoot_direction_lock = direction_lock::UNLOCKED;
    //    //result.x_movement = 0.0f;
    //    if (direction.x < 0.0f)
    //    {
    //        result.x_movement = -FLT_MIN;
    //    }
    //    else if (direction.x > 0.0f)
    //    {
    //        result.x_movement = FLT_MIN;
    //    }
    //}
    //else
    //{
    //    result.x_movement = 0.0f;
    //    result.shoot_direction_lock = direction_lock::LOCKED;
    //    result.shoot_direction = Vector2F(0.0f);
    //}

    result.connection = connection_state::CONNECTED;

    result.left_analog_stick = current.left_analog_stick;
    result.right_analog_stick = current.right_analog_stick;

    return result;
}

std::vector<player_input> PlayerInput::update_and_get_player_inputs()
{
    raw_player_input current[4];
    for (int i = 0; i < 4; i++)
    {
        current[i] = this->get_raw_input(i);
    }
    std::vector<player_input> result;
    for (int i = 0; i < 4; i++)
    {
        if (current[i].connected)
        {
            player_input input = calculate_player_input(current[i],
                this->_prev_inputs[i]);
            result.push_back(input);
        }
        this->_prev_inputs[i] = current[i];
    }
    return result;
}