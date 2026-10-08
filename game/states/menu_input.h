#pragma once

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/ui/navigation.h"
#include "game/input_slots.h"
#include "game/states/menu_input_action.h"
#include <vector>

class MenuLevelSettings;

namespace menu_input_consts
{
	// Return-crossing thresholds for consecutive raw stick samples.
	// Equal values provide no hysteresis.
	constexpr float digital_presence_threshold = 0.8f;
	constexpr float digital_absence_threshold = 0.8f;

	// Menu button mappings; arming flags use these table indices.
	struct ActionButton
	{
		labrador::GamepadButton button;
		MenuInputAction action;
	};

	constexpr int action_button_count = 3;

	constexpr ActionButton action_buttons[action_button_count] =
	{
		{ labrador::GamepadButton::a, MenuInputAction::proceed },
		{ labrador::GamepadButton::b, MenuInputAction::back },
		{ labrador::GamepadButton::start, MenuInputAction::pause },
	};

	// Each action has primary and alternate keys and one shared arming flag.
	// Escape maps to back, which also resumes the pause menu.
	struct ActionKey
	{
		labrador::Key key;
		labrador::Key alternate;
		MenuInputAction action;
	};

	constexpr int action_key_count = 2;

	constexpr ActionKey action_keys[action_key_count] =
	{
		{ labrador::Key::enter, labrador::Key::space,
			MenuInputAction::proceed },
		{ labrador::Key::escape, labrador::Key::backspace,
			MenuInputAction::back },
	};

	// Menu navigation accepts arrows and WASD.
	struct DirectionKey
	{
		labrador::Key key;
		labrador::Key alternate;
		labrador::Direction direction;
	};

	constexpr int direction_key_count = 4;

	constexpr DirectionKey direction_keys[direction_key_count] =
	{
		{ labrador::Key::up, labrador::Key::w, labrador::Direction::up },
		{ labrador::Key::down, labrador::Key::s, labrador::Direction::down },
		{ labrador::Key::left, labrador::Key::a, labrador::Direction::left },
		{ labrador::Key::right, labrador::Key::d, labrador::Direction::right },
	};
}

struct ProcessedMenuInput
{
	labrador::Direction direction = labrador::Direction::none;
	MenuInputAction action = MenuInputAction::none;
	bool connected = false;
};

// Maps engine device state to menu navigation and armed action releases.
class MenuInput
{
public:
	MenuInput(const labrador::Gamepads* gamepads,
		const labrador::Keyboard* keyboard);

	// Returns input_slots::count entries indexed by device slot; absent devices
	// are neutral. Actions fire on release only if their press was observed here.
	// Call during update: this advances arming, but repeated reads of the same
	// frame return the same result.
	std::vector<ProcessedMenuInput> menu_inputs();
	// Same input, in the fixed roster's player order. A missing device leaves
	// its own row neutral instead of handing the row to another controller.
	std::vector<ProcessedMenuInput> player_inputs(const MenuLevelSettings& settings);

	// Clear ownership when taking input back from gameplay or another context.
	void reset_actions();

private:
	ProcessedMenuInput menu_input(const labrador::Gamepads& gamepads,
		int slot);
	ProcessedMenuInput keyboard_menu_input(const labrador::Keyboard& keyboard);

	const labrador::Gamepads* gamepads_ = nullptr;
	const labrador::Keyboard* keyboard_ = nullptr;

	// Per slot and action: whether this menu observed the current press.
	// This prevents releasing a gameplay-held button from activating a new menu.
	// reset_actions() clears ownership when input changes context.
	bool armed_[input_slots::pad_count]
		[menu_input_consts::action_button_count] = {};

	// Keyboard arming is indexed by action_keys, not the pad button table.
	bool key_armed_[menu_input_consts::action_key_count] = {};
};
