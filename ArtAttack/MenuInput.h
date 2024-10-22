#ifndef MENUINPUT_H
#define MENUINPUT_H

#include <vector>
#include "menu_input_action.h"
#include "connection_state.h"
#include "MattMath.h"

namespace menu_input_consts
{
	const float DIGITAL_PRESENCE_THRESHOLD = 0.8f;
	const float DIGITAL_ABSENCE_THRESHOLD = 0.8f;
}

struct raw_menu_input
{
	MattMath::Vector2F left_analog_stick = MattMath::Vector2F::ZERO;
	bool proceed = false;
	bool back = false;
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool pause = false;
	bool connected = false;
};

enum class menu_direction
{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	NONE
};

struct menu_input
{
	menu_direction direction = menu_direction::NONE;
	menu_input_action action = menu_input_action::NONE;
	connection_state connection = connection_state::DISCONNECTED;
};

class MenuInput
{
private:
	raw_menu_input _prev_inputs[4];
	raw_menu_input get_raw_input(int gamepad_num);
	menu_input calculate_menu_input(const raw_menu_input& current,
		const raw_menu_input& previous);
	DirectX::GamePad* _gamepad = nullptr;
public:
	MenuInput(DirectX::GamePad* gamepad);
	std::vector<menu_input> update_and_get_menu_inputs();
};

#endif // !MENUINPUT_H
