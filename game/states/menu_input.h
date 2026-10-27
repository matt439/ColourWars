#pragma once

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/ui/navigation.h"
#include "game/input_slots.h"
#include "game/action_bindings.h"
#include "game/tuning.h"
#include "game/states/menu_input_action.h"
#include <array>
#include <vector>

class MenuLevelSettings;

// One device's held navigation, blocked until neutral after a context change.
class MenuDirectionRepeat
{
public:
	labrador::Direction update(const std::array<bool, 4>& held, float dt,
		float delay, float interval);
	labrador::Direction update_pad(const labrador::GamepadState& pad, float dt,
		float delay, float interval, const InputTuning& tuning, bool use_stick = true);
private:
	bool ready_ = false;
	labrador::DirectionRepeat repeat_;
	std::array<bool, 4> stick_ = {};
};

struct ProcessedMenuInput
{
	labrador::Direction direction = labrador::Direction::none;
	MenuInputAction action = MenuInputAction::none;
	bool connected = false;
};

// Retain the opening device while connected; for a seat, that is the pad the
// seat reads now (MenuLevelSettings::device_slot). Otherwise accept another match
// participant, or any device if all participants are missing. Invalid owners
// return neutral input.
ProcessedMenuInput menu_input_for_owner(
	const std::vector<ProcessedMenuInput>& inputs, int opening_slot,
	const MenuLevelSettings& settings);

// Maps engine device state to menu navigation and armed action releases.
class MenuInput
{
public:
	MenuInput(const labrador::Gamepads* gamepads,
		const labrador::Keyboard* keyboard, const GameTuning* tuning);

	// Advance navigation once before a page update; reads below do not advance time.
	void update_directions(float dt, float delay, float interval, bool use_stick = true);
	void reset_directions();

	// Returns input_slots::count entries indexed by device slot; absent devices
	// are neutral. Actions fire on release only if their press was observed here.
	// Call during update: this advances arming, but repeated reads of the same
	// frame return the same result.
	std::vector<ProcessedMenuInput> menu_inputs();
	// Same input, in the fixed roster's player order. A missing device leaves
	// its own row neutral instead of handing the row to another controller.
	// Explicit bots are connected and neutral; pages route host choices to them.
	std::vector<ProcessedMenuInput> player_inputs(const MenuLevelSettings& settings);

	// Clear ownership when taking input back from gameplay or another context.
	void reset_actions();
	// The same for one pad slot only; the keyboard and other pads keep theirs.
	void reset_pad_actions(int slot);

private:
	ProcessedMenuInput menu_input(const labrador::Gamepads& gamepads,
		int slot);
	ProcessedMenuInput keyboard_menu_input(const labrador::Keyboard& keyboard);

	const labrador::Gamepads* gamepads_ = nullptr;
	const labrador::Keyboard* keyboard_ = nullptr;
	const GameTuning* tuning_ = nullptr;

	// Per slot and action: whether this menu observed the current press.
	// This prevents releasing a gameplay-held button from activating a new menu.
	// reset_actions() clears ownership when input changes context.
	bool armed_[input_slots::pad_count]
		[menu_action_count] = {};

	MenuDirectionRepeat navigation_[input_slots::count];
	labrador::Direction directions_[input_slots::count] = {};

	// Each logical action owns its aliases; the unbound keyboard Pause stays neutral.
	bool key_armed_[menu_action_count] = {};
};
