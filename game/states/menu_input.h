#pragma once

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/ui/navigation.h"
#include "game/input_slots.h"
#include "game/states/menu_input_action.h"
#include <vector>

namespace menu_input_consts
{
	// The stick has to pass the first threshold and then fall back inside the
	// second for a direction to count, so a held stick is one step and not a
	// stream of them. They are equal today; they are two names because the pair
	// is a hysteresis band and collapsing them would make a stick resting on
	// the line stutter.
	constexpr float digital_presence_threshold = 0.8f;
	constexpr float digital_absence_threshold = 0.8f;

	// The buttons a menu action can come from, and what each one means.
	//
	// A table rather than a chain because MenuInput's arming is indexed by
	// position in it: a second list of the same three buttons would be free to
	// drift from this one, and the arming would then be reading someone else's
	// flag.
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

	// The keyboard's menu bindings, the same table for the same reason: the
	// arming below is indexed by position in it.
	//
	// Two keys per action, because a keyboard has an obvious answer and a
	// conventional one and there is no cost to honouring both. There is no
	// `pause` row: PauseMenuInitial treats back and pause identically, so
	// Escape closing the pause menu is already `back`, and a second key
	// meaning almost-back would be a distinction with nothing behind it.
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

	// One row per direction, arrows and WASD. W and S are here and A and D are
	// the walking keys in a match, which is not a clash: nothing reads both
	// tables in one frame, because a menu is either up or it is not.
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
	// labrador::Direction, not a game enum: which way the stick went is the
	// input to engine/ui's navigation walk, and there is no version of it that
	// belongs to this game.
	labrador::Direction direction = labrador::Direction::none;
	MenuInputAction action = MenuInputAction::none;
	bool connected = false;
};

// What the pads mean to a menu, one entry per slot.
//
// A mapping and almost nothing else, for the same reason PlayerInput is: the
// pads and the edges between frames belong to the engine
// (engine/input/gamepads.h). The polling, the previous-frame store and the
// prime() that used to live here were the second of two copies, and they had
// already drifted from the first. What is left is armed_, and it is not prime()
// coming back - see the comment on it.
class MenuInput
{
public:
	MenuInput(const labrador::Gamepads* gamepads,
		const labrador::Keyboard* keyboard);

	// Returns exactly input_slots::count entries, indexed by slot. Absent
	// inputs carry neutral data.
	//
	// Every action here is a *release* edge: the button has to come back up,
	// and it has to have gone down while a menu was reading the device.
	//
	// Not const, because it advances armed_ - but calling it twice in one
	// frame answers the same thing twice, which a page that reads the pads
	// through more than one helper depends on. It is still an update()-time
	// call and not a draw()-time one: draw() runs on every render worker at
	// once.
	std::vector<ProcessedMenuInput> menu_inputs();

private:
	ProcessedMenuInput menu_input(const labrador::Gamepads& gamepads,
		int slot);
	ProcessedMenuInput keyboard_menu_input(const labrador::Keyboard& keyboard);

	const labrador::Gamepads* gamepads_ = nullptr;
	const labrador::Keyboard* keyboard_ = nullptr;

	// Per pad slot, per menu_input_consts::action_buttons entry: did this
	// MenuInput watch the button's current down-run start?
	//
	// WHY A MENU NEEDS THIS. Gamepads is polled once a frame whatever is on top
	// of the stack, which is what makes an edge honest - and is also what hands
	// a page pushed under a held button that button's release as if it were its
	// own. Hold A to jump and press Start with the same hand: Start's release
	// opens the pause menu, and A's release, a frame later, activates whatever
	// the menu opened onto. So a release only answers if the press that began
	// it was seen from here, and a press made while a match was on top was not.
	//
	// It is not prime() coming back. prime() had to be called by hand at every
	// transition, which is what let it drift from PlayerInput's copy; nothing
	// calls this at a transition, because nothing but a menu ever calls
	// menu_inputs().
	bool armed_[input_slots::pad_count]
		[menu_input_consts::action_button_count] = {};

	// The same flag for the keyboard's two action keys, and it is a separate
	// array rather than a fifth row because the two tables are different
	// lengths and indexing one by the other's position is the bug this whole
	// mechanism exists to avoid.
	bool key_armed_[menu_input_consts::action_key_count] = {};
};
