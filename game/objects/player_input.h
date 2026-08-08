#pragma once

#include <vector>
#include "game/objects/player_input_data.h"
#include "engine/math/matt_math.h"

namespace player_input_consts
{
	constexpr float TRIGGER_SHOOT_THRESHOLD = 0.6f;
	constexpr float TRIGGER_JUMP_THRESHOLD = 0.6f;
	constexpr float STICK_DEADZONE = 0.5f;

	// XInput slots. An index into the vector returned by
	// update_and_get_player_inputs() is a pad slot, never an ordinal.
	constexpr int MAX_PAD_COUNT = 4;
}

struct RawPlayerInput
{
	mattmath::Vector2F left_analog_stick = { 0.0f, 0.0f };
	mattmath::Vector2F right_analog_stick = { 0.0f, 0.0f };
	bool jump_button = false;
	float jump_trigger = 0.0f;
	float primary_shoot = 0.0f;
	bool secondary_shoot = false;
	bool show_debug = false;
	bool pause_menu = false;
	bool connected = false;
};

class PlayerInput
{
public:
	explicit PlayerInput(DirectX::GamePad* gamepad);

	// Returns exactly player_input_consts::MAX_PAD_COUNT entries, indexed by
	// XInput pad slot. Absent pads carry ConnectionState::disconnected and
	// neutral input rather than being omitted.
	//
	// jump is a *press* edge and pause/debug are *release* edges, all computed
	// against the previous frame. Read that together with prime().
	std::vector<PlayerInputData> update_and_get_player_inputs();

	// Seeds the previous-frame state from the pads as they are right now.
	//
	// Call it whenever gameplay becomes the thing reading input. "Previous"
	// only advances on frames update_and_get_player_inputs() is called, and
	// that stops while a menu is up - so a player who paused mid-jump came
	// back with previous.jump_button still true, and
	// `current && !previous` swallowed their first jump on the way out.
	//
	// Same defect as MenuInput::prime(), in the other direction: an edge
	// detector whose "previous" only advances on frames its owner happens to
	// be running.
	void prime();
private:
	RawPlayerInput prev_inputs_[player_input_consts::MAX_PAD_COUNT];
	RawPlayerInput raw_input(int gamepad_num) const;
	static PlayerInputData calculate_player_input(const RawPlayerInput& current,
	                                              const RawPlayerInput& previous);
	DirectX::GamePad* gamepad_ = nullptr;
};
