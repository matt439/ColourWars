#pragma once

#include "engine/input/gamepad.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "engine/ui/navigation.h"
#include "game/states/menu_input_action.h"
#include <array>
#include <cstddef>
#include <vector>

struct InputTuning;

namespace player_input_consts
{
	// Retained names also define the developer bind-warning policy.
	constexpr labrador::Key move_left_key = labrador::Key::a;
	constexpr labrador::Key move_right_key = labrador::Key::d;
	constexpr labrador::Key jump_key = labrador::Key::space;
	constexpr labrador::Key drop_key = labrador::Key::s;
	constexpr labrador::Key drop_key_alternate = labrador::Key::down;
	constexpr labrador::Key pause_key = labrador::Key::escape;
	constexpr labrador::Key debug_key = labrador::Key::f1;
	constexpr labrador::Key confirm_key = labrador::Key::enter;
	constexpr labrador::MouseButton shoot_button = labrador::MouseButton::left;
}

enum class GameAction
{
	move_left,
	move_right,
	jump,
	shoot,
	drop,
	pause,
	debug,
	skip_intro,
	count
};

struct ActionSample
{
	bool held = false;
	bool pressed = false;
	bool released = false;
	bool was_held = false;
};

enum class ActionTriggerThreshold { jump, shoot };

struct TriggerActionBinding
{
	labrador::GamepadTrigger trigger = labrador::GamepadTrigger::left;
	ActionTriggerThreshold threshold = ActionTriggerThreshold::jump;
};

struct DesktopActionBinding
{
	std::vector<labrador::Key> keys;
	std::vector<labrador::MouseButton> mouse_buttons;
};

struct PadActionBinding
{
	std::vector<labrador::GamepadButton> buttons;
	std::vector<TriggerActionBinding> triggers;
};

struct MenuDirectionBinding
{
	labrador::Direction direction = labrador::Direction::none;
	std::vector<labrador::Key> keys;
	labrador::GamepadButton button = labrador::GamepadButton::dpad_up;
};

inline constexpr std::size_t game_action_count = static_cast<std::size_t>(GameAction::count);
inline constexpr int menu_action_count = static_cast<int>(MenuInputAction::none);
inline constexpr int menu_direction_count = 4;

// Only existing digital/trigger mappings. Stick response, the held stick-Y
// drop comparison, menu repeat/hysteresis and input ownership stay in readers.
struct ActionBindings
{
	std::array<DesktopActionBinding, game_action_count> game_desktop;
	std::array<PadActionBinding, game_action_count> game_pad;
	// Indexed by MenuInputAction; the default keyboard Pause entry is empty.
	std::array<std::vector<labrador::Key>, menu_action_count> menu_keys;
	std::array<std::vector<labrador::GamepadButton>, menu_action_count> menu_buttons;
	// Fixed order: up, down, left, right. Readers retain direction conflict rules.
	std::array<MenuDirectionBinding, menu_direction_count> directions;
};

const ActionBindings& default_action_bindings();

// Pure snapshot evaluations. Held is neutral without current focus/connection;
// edges use the engine's rules, including the mouse button epoch. Aliases OR
// each source's held/pressed/released separately, rather than deriving an edge
// from their combined held state. Invalid action indices throw out_of_range.
ActionSample game_action(const ActionBindings& bindings, GameAction action,
	const labrador::KeyboardState& keyboard, const labrador::KeyboardState& previous_keyboard,
	const labrador::MouseState& mouse, const labrador::MouseState& previous_mouse);
ActionSample game_action(const ActionBindings& bindings, GameAction action,
	const labrador::GamepadState& pad, const labrador::GamepadState& previous_pad,
	const InputTuning& tuning);

// Menu keyboard was_held preserves the legacy per-key reconstruction
// (held && !pressed) || released, then ORs aliases. Thus a held key on focus
// return can report was_held; the reader still clears ownership on that frame.
// Menu pad was_held is the previous button level, with reconnect ownership and
// the all-actions-up/one-previous-action release gate retained by the reader.
ActionSample menu_action(const ActionBindings& bindings, MenuInputAction action,
	const labrador::KeyboardState& keyboard, const labrador::KeyboardState& previous_keyboard);
ActionSample menu_action(const ActionBindings& bindings, MenuInputAction action,
	const labrador::GamepadState& pad, const labrador::GamepadState& previous_pad);

std::array<bool, menu_direction_count> keyboard_direction_held(
	const ActionBindings& bindings, const labrador::KeyboardState& keyboard);
std::array<bool, menu_direction_count> dpad_direction_held(
	const ActionBindings& bindings, const labrador::GamepadState& pad);
