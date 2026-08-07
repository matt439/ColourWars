#include "game/pch.h"
#include "game/states/menu_input.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_input_consts;

MenuInput::MenuInput(GamePad* gamepad) :
    gamepad_(gamepad)
{
    for (auto& prev_input : this->prev_inputs_)
    {
	    prev_input = RawMenuInput();
    }
}

RawMenuInput MenuInput::raw_input(int gamepad_num) const
{
	auto result = RawMenuInput();
	const auto pad = this->gamepad_->GetState(gamepad_num, GamePad::DEAD_ZONE_NONE);
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

ProcessedMenuInput MenuInput::calculate_menu_input(const RawMenuInput& current,
    const RawMenuInput& previous)
{
	auto result = ProcessedMenuInput();
    bool no_press_current = !current.proceed && !current.back && !current.pause;
    bool prev_proceed = previous.proceed && !previous.back && !previous.pause;
    bool prev_back = previous.back && !previous.proceed && !previous.pause;
    bool prev_pause = previous.pause && !previous.proceed && !previous.back;
    bool proceed = prev_proceed && no_press_current;
    bool back = prev_back && no_press_current;
    bool pause = prev_pause && no_press_current;

    if (proceed)
    {
        result.action = MenuInputAction::proceed;
    }
    else if (back)
    {
        result.action = MenuInputAction::back;
    }
	else if (pause)
	{
		result.action = MenuInputAction::pause;
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
        result.action = MenuInputAction::none;
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
        result.direction = MenuDirection::up;
    }
    else if (down)
    {
        result.direction = MenuDirection::down;
    }
    else if (left)
    {
        result.direction = MenuDirection::left;
    }
    else if (right)
    {
        result.direction = MenuDirection::right;
    }
    else if (up && down && left && right)
    {
        throw new std::exception(
            "up, down, left, and right pressed at same time");
    }
    else
    {
        result.direction = MenuDirection::none;
    }
    result.connection = ConnectionState::connected;
    return result;
}

std::vector<ProcessedMenuInput> MenuInput::update_and_get_menu_inputs()
{
    RawMenuInput current[MAX_PAD_COUNT];
    for (int i = 0; i < MAX_PAD_COUNT; i++)
    {
        current[i] = this->raw_input(i);
    }

    // One entry per pad slot, always - see PlayerInput for why. Slots with no
    // pad keep the default value (direction NONE, action NONE, DISCONNECTED),
    // which every menu loop already treats as "no input this frame".
    std::vector<ProcessedMenuInput> result(MAX_PAD_COUNT);
    for (int i = 0; i < MAX_PAD_COUNT; i++)
    {
        if (current[i].connected)
        {
            result[i] = calculate_menu_input(current[i], this->prev_inputs_[i]);
        }
        this->prev_inputs_[i] = current[i];
    }
    return result;
}
