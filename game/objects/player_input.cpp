#include "game/pch.h"
#include "game/objects/player_input.h"
#include "game/input_slots.h"
#include "game/objects/stick_response.h"

using namespace mattmath;
using namespace labrador;

PlayerInput::PlayerInput(const Gamepads* gamepads, const Keyboard* keyboard,
	const Mouse* mouse, const GameTuning* tuning, const PlaySettings* preferences,
	const InputBindings* input_bindings) :
	gamepads_(gamepads), keyboard_(keyboard), mouse_(mouse), tuning_(tuning), preferences_(preferences),
	input_bindings_(input_bindings)
{
	if (this->input_bindings_ == nullptr) { throw std::invalid_argument("PlayerInput requires input bindings"); }
	this->keyboard_bindings_ = effective_action_bindings(*this->input_bindings_);
	this->keyboard_jump_ = this->input_bindings_->keyboard_jump;
	this->pad_jump_ = this->input_bindings_->pad_jump;
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		this->pad_bindings_[slot] = effective_action_bindings(*this->input_bindings_, slot);
	}
}

const ActionBindings& PlayerInput::effective_bindings() const
{
	if (this->keyboard_jump_ != this->input_bindings_->keyboard_jump)
	{
		this->keyboard_bindings_ = effective_action_bindings(*this->input_bindings_);
		this->keyboard_jump_ = this->input_bindings_->keyboard_jump;
	}
	return this->keyboard_bindings_;
}

const ActionBindings& PlayerInput::effective_pad_bindings(int slot) const
{
	if (this->pad_jump_[slot] != this->input_bindings_->pad_jump[slot])
	{
		this->pad_bindings_[slot] = effective_action_bindings(*this->input_bindings_, slot);
		this->pad_jump_[slot] = this->input_bindings_->pad_jump[slot];
	}
	return this->pad_bindings_[slot];
}

PlayerInputData PlayerInput::pad_input(const Gamepads& gamepads, int slot) const
{
	const GamepadState& pad = gamepads.state(slot);
	const GamepadState& before = gamepads.previous_state(slot);
	const ActionBindings& bindings = this->effective_pad_bindings(slot);
	const auto action = [&](GameAction name)
	{
		return game_action(bindings, name, pad, before, this->tuning_->input());
	};

	PlayerInputData result;
	result.connected = true;

	// Jump uses this slot's face button or LT crossing its threshold. Suppress press
	// edges on reconnect: the previous state is neutral, so an already-held
	// button would otherwise look newly pressed. Preserve the held state.
	const ActionSample jump = action(GameAction::jump);
	result.jump_held = jump.held;
	result.jump_pressed = !gamepads.just_connected(slot) && jump.pressed;
	result.primary_shoot = action(GameAction::shoot).held;

	// Open menus on release so the opening button is no longer held.
	// MenuInput arming also handles other buttons held during the transition.
	const ActionSample pause = action(GameAction::pause);
	result.toggle_pause_menu = pause.released;
	result.pause_held = pause.held;
	result.toggle_debug = action(GameAction::debug).released;
	result.skip_intro = action(GameAction::skip_intro).released;

	const StickResponse response = stick_response(pad.left_stick, pad.right_stick,
		this->tuning_->input().movement_deadzone, *this->preferences_, slot);
	result.x_movement = response.movement;

	// Map the downward stick threshold to a device-independent drop action.
	result.drop_through = pad.left_stick.y >
		this->tuning_->player().drop_down_analog_threshold;

	// Apply a radial deadzone, then retain only the non-zero aim direction.
	const Vector2F aim = response.aim;
	result.shoot_direction_requested = aim != Vector2F::ZERO;
	if (result.shoot_direction_requested)
	{
		result.shoot_direction = aim.normalized();
		result.shoot_angle = result.shoot_direction.angle();
	}

	result.left_analog_stick = pad.left_stick;
	result.right_analog_stick = pad.right_stick;

	return result;
}

PlayerInputData PlayerInput::keyboard_and_mouse_input(const Keyboard& keyboard,
	const Mouse& mouse) const
{
	const ActionBindings& bindings = this->effective_bindings();
	const auto action = [&](GameAction name)
	{
		return game_action(bindings, name, keyboard.state(), keyboard.previous_state(),
			mouse.state(), mouse.previous_state());
	};
	PlayerInputData result;
	result.connected = true;

	// Opposing movement keys cancel.
	const float left = action(GameAction::move_left).held ? 1.0f : 0.0f;
	const float right = action(GameAction::move_right).held ? 1.0f : 0.0f;
	result.x_movement = right - left;

	const ActionSample jump = action(GameAction::jump);
	result.jump_held = jump.held;
	result.jump_pressed = jump.pressed;
	result.primary_shoot = action(GameAction::shoot).held;

	// Dropping is a held request, checked when the player is on a pass-through platform.
	result.drop_through = action(GameAction::drop).held;

	// Match the pad's release edges so the opening key is no longer held.
	const ActionSample pause = action(GameAction::pause);
	result.toggle_pause_menu = pause.released;
	result.pause_held = pause.held;
	result.toggle_debug = action(GameAction::debug).released;
	result.skip_intro = action(GameAction::skip_intro).released;

	// Level fills mouse aim after converting the player centre to screen coordinates.

	// Leave raw stick positions zero; the debug overlay should not invent stick input.

	return result;
}

std::vector<PlayerInputData> PlayerInput::player_inputs() const
{
	this->input_bindings_->validate();
	std::vector<PlayerInputData> result(input_slots::count);
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (this->gamepads_->connected(slot))
		{
			result[slot] = this->pad_input(*this->gamepads_, slot);
		}
	}

	// Always expose the keyboard/mouse slot, independent of recent activity.
	result[input_slots::keyboard_and_mouse] =
		this->keyboard_and_mouse_input(*this->keyboard_, *this->mouse_);

	return result;
}
