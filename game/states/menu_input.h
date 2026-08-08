#pragma once

#include <vector>
#include "game/states/menu_input_action.h"
#include "engine/input/connection_state.h"
#include "engine/ui/navigation.h"
#include "engine/math/matt_math.h"
#include "game/objects/player_input.h"

namespace menu_input_consts
{
	constexpr float DIGITAL_PRESENCE_THRESHOLD = 0.8f;
	constexpr float DIGITAL_ABSENCE_THRESHOLD = 0.8f;

	using player_input_consts::MAX_PAD_COUNT;
}

struct RawMenuInput
{
	mattmath::Vector2F left_analog_stick = mattmath::Vector2F::ZERO;
	bool proceed = false;
	bool back = false;
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool pause = false;
	bool connected = false;
};

struct ProcessedMenuInput
{
	// artattack::Direction, not a game enum: which way the stick went is the
	// input to engine/ui's navigation walk, and there is no version of it that
	// belongs to this game. The pad polling and the edge detection below still
	// live here - moving those is the input module's job (PLAN.md D2), and
	// they are written twice in this repository already.
	artattack::Direction direction = artattack::Direction::none;
	MenuInputAction action = MenuInputAction::none;
	artattack::ConnectionState connection = artattack::ConnectionState::disconnected;
};

class MenuInput
{
public:
	explicit MenuInput(DirectX::GamePad* gamepad);

	// Returns exactly menu_input_consts::MAX_PAD_COUNT entries, indexed by
	// XInput pad slot. Absent pads carry DISCONNECTED and neutral input.
	//
	// Every action here is a *release* edge, computed against the previous
	// frame. That contract was written down nowhere, which is how the bug
	// prime() fixes survived: read the two together.
	std::vector<ProcessedMenuInput> update_and_get_menu_inputs();

	// Seeds the previous-frame state from the pads as they are right now.
	//
	// Call it whenever this input becomes the one being read. "Previous" only
	// advances on frames update_and_get_menu_inputs() is called, which is only
	// while a menu is on screen - so without priming, a button already held
	// when the menu opens is seen as a press the menu itself received, and its
	// release is delivered as an action.
	//
	// It was reachable in one sitting: A is jump, so holding A and tapping
	// Start opened the pause menu, and the pause menu closed itself the
	// instant the player let go of jump - confirming its default row, Resume.
	// After priming, a button held across a transition must be released and
	// pressed again to count.
	void prime();
private:
	RawMenuInput prev_inputs_[menu_input_consts::MAX_PAD_COUNT];
	RawMenuInput raw_input(int gamepad_num) const;
	static ProcessedMenuInput calculate_menu_input(const RawMenuInput& current,
	                                               const RawMenuInput& previous);
	DirectX::GamePad* gamepad_ = nullptr;
};
