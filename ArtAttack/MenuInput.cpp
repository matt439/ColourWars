#include "pch.h"
#include "MenuInput.h"

using namespace DirectX;
//using namespace DirectX::SimpleMath;
using namespace MattMath;

MenuInput::MenuInput(GamePad* gamepad) :
    _gamepad(gamepad)
{
    for (int i = 0; i < 4; i++)
    {
        this->_prev_inputs[i] = raw_menu_input();
    }
}

raw_menu_input MenuInput::get_raw_input(int gamepad_num)
{
    raw_menu_input result = raw_menu_input();
    auto pad = this->_gamepad->GetState(gamepad_num, GamePad::DEAD_ZONE_NONE);
    if (pad.IsConnected())
    {
        result.left_analog_stick = Vector2F(pad.thumbSticks.leftX,
            -pad.thumbSticks.leftY);
        result.proceed = pad.IsAPressed();
        result.back = pad.IsBPressed();
        result.up = pad.IsDPadUpPressed();
        result.down = pad.IsDPadDownPressed();
        result.left = pad.IsDPadLeftPressed();
        result.right = pad.IsDPadRightPressed();
        result.pause = pad.IsStartPressed();
        result.connected = true;
    }
    return result;
}

menu_input MenuInput::calculate_menu_input(const raw_menu_input& current,
    const raw_menu_input& previous)
{
    menu_input result = menu_input();
    bool no_press_current = !current.proceed && !current.back && !current.pause;
    bool prev_proceed = previous.proceed && !previous.back && !previous.pause;
    bool prev_back = previous.back && !previous.proceed && !previous.pause;
    bool prev_pause = previous.pause && !previous.proceed && !previous.back;
    bool proceed = prev_proceed && no_press_current;
    bool back = prev_back && no_press_current;
    bool pause = prev_pause && no_press_current;
    //bool proceed = previous.proceed && !current.proceed;
    //bool back = previous.back && !current.back;
    if (proceed)
    {
        result.action = menu_input_action::PROCEED;
    }
    else if (back)
    {
        result.action = menu_input_action::BACK;
    }
	else if (pause)
	{
		result.action = menu_input_action::PAUSE;
	}
    else if ((proceed && back) ||
        (proceed && pause) ||
        (pause && back))
    {
        throw new std::exception(
            "2 or more of proceed, back or pause pressed at same time");
    }
    else
    {
        result.action = menu_input_action::NONE;
    }

    bool analog_down = previous.left_analog_stick.y >
        menu_input_consts::DIGITAL_PRESENCE_THRESHOLD &&
            current.left_analog_stick.y <
            menu_input_consts::DIGITAL_ABSENCE_THRESHOLD;
    bool analog_up = previous.left_analog_stick.y <
        -menu_input_consts::DIGITAL_PRESENCE_THRESHOLD &&
            current.left_analog_stick.y >
            -menu_input_consts::DIGITAL_ABSENCE_THRESHOLD;
    bool analog_left = previous.left_analog_stick.x <
        -menu_input_consts::DIGITAL_PRESENCE_THRESHOLD &&
            current.left_analog_stick.x >
            -menu_input_consts::DIGITAL_ABSENCE_THRESHOLD;
    bool analog_right = previous.left_analog_stick.x >
        menu_input_consts::DIGITAL_PRESENCE_THRESHOLD &&
            current.left_analog_stick.x <
            menu_input_consts::DIGITAL_ABSENCE_THRESHOLD;


    bool digital_no_direction_press_current = !current.up &&
        !current.down && !current.left && !current.right;
    bool digital_up = previous.up && digital_no_direction_press_current;
    bool digital_down = previous.down && digital_no_direction_press_current;
    bool digital_left = previous.left && digital_no_direction_press_current;
    bool digital_right = previous.right && digital_no_direction_press_current;

    bool up = analog_up || digital_up;
    bool down = analog_down || digital_down;
    bool left = analog_left || digital_left;
	bool right = analog_right || digital_right;

    if (up)
    {
        result.direction = menu_direction::UP;
    }
    else if (down)
    {
        result.direction = menu_direction::DOWN;
    }
    else if (left)
    {
        result.direction = menu_direction::LEFT;
    }
    else if (right)
    {
        result.direction = menu_direction::RIGHT;
    }
    else if (up && down && left && right)
    {
        throw new std::exception(
            "up, down, left, and right pressed at same time");
    }
    else
    {
        result.direction = menu_direction::NONE;
    }
    result.connection = connection_state::CONNECTED;
    return result;
}

std::vector<menu_input> MenuInput::update_and_get_menu_inputs()
{
    raw_menu_input current[4];
    for (int i = 0; i < 4; i++)
    {
        current[i] = this->get_raw_input(i);
    }
    std::vector<menu_input> result;
    for (int i = 0; i < 4; i++)
    {
        if (current[i].connected)
        {
            menu_input input =
                calculate_menu_input(current[i], this->_prev_inputs[i]);
            result.push_back(input);
        }
        this->_prev_inputs[i] = current[i];
    }
    return result;
}