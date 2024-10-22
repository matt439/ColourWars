#ifndef MENUINPUT_H
#define MENUINPUT_H

#include <vector>
#include "menu_input_action.h"
#include "connection_state.h"
#include "MattMath.h"

namespace menu_input_consts
{
	constexpr float DIGITAL_PRESENCE_THRESHOLD = 0.8f;
	constexpr float DIGITAL_ABSENCE_THRESHOLD = 0.8f;
}

struct RawMenuInput
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

struct ProcessedMenuInput
{
	menu_direction direction = menu_direction::NONE;
	menu_input_action action = menu_input_action::NONE;
	connection_state connection = connection_state::DISCONNECTED;
};

class MenuInput
{
public:
	explicit MenuInput(DirectX::GamePad* gamepad);
	std::vector<ProcessedMenuInput> update_and_get_menu_inputs();
private:
	RawMenuInput _prev_inputs[4];
	RawMenuInput get_raw_input(int gamepad_num) const;
	static ProcessedMenuInput calculate_menu_input(const RawMenuInput& current,
	                                               const RawMenuInput& previous);
	DirectX::GamePad* _gamepad = nullptr;
};

#endif // !MENUINPUT_H
