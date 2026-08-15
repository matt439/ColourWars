#pragma once

#include "engine/input/gamepads.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "game/objects/player_input_data.h"
#include "game/tuning.h"
#include <vector>

namespace player_input_consts
{
	constexpr float trigger_shoot_threshold = 0.6f;
	constexpr float trigger_jump_threshold = 0.6f;

	// Applied once, radially, and rescaled - see engine/input/gamepad.h. It
	// used to be applied on top of a circular deadzone the pad API had already
	// applied, without rescaling, so a stick had half its travel dead and the
	// other half started at half speed.
	constexpr float stick_deadzone = 0.5f;

	// The keyboard half of the keyboard-and-mouse slot.
	//
	// Spelt as constants rather than inline so that the bindings are a list
	// somebody can read, and A/D rather than left/right arrow because the
	// other hand is on the mouse. There is no rebinding screen, so these are
	// in code and not in data - which is the engine's own argument for having
	// no action-mapping layer (engine/input/gamepad.h), and it applies here
	// for as long as it stays true.
	constexpr labrador::Key move_left_key = labrador::Key::a;
	constexpr labrador::Key move_right_key = labrador::Key::d;
	constexpr labrador::Key jump_key = labrador::Key::space;

	// Down, for falling through a jump-through platform. Two keys because the
	// other four movement bindings already answer to both conventions, and S
	// is the one a hand on A/D is already resting beside.
	constexpr labrador::Key drop_key = labrador::Key::s;
	constexpr labrador::Key drop_key_alternate = labrador::Key::down;
	constexpr labrador::Key pause_key = labrador::Key::escape;
	constexpr labrador::Key debug_key = labrador::Key::f1;

	constexpr labrador::MouseButton shoot_button = labrador::MouseButton::left;
}

// What the pads mean to a match, one entry per slot.
//
// This is now a mapping and nothing else: no polling, no previous-frame store,
// no priming. The engine holds the pads and the edges (engine/input/gamepads.h)
// because an edge is only right when "previous" is genuinely last frame, which
// it cannot be for a detector that only runs while gameplay is on screen.
class PlayerInput
{
public:
	// The tuning is here for one number: the threshold a stick has to pass to
	// mean "drop through". That comparison used to live in Player, which is
	// what made the action unreachable without a stick.
	PlayerInput(const labrador::Gamepads* gamepads,
		const labrador::Keyboard* keyboard,
		const labrador::Mouse* mouse,
		const GameTuning* tuning);

	// Returns exactly input_slots::count entries, indexed by slot. Absent
	// inputs carry neutral data rather than being omitted: the index is a slot
	// and every caller reads it as a player number, so a vector that skipped
	// empty slots would hand a player someone else's controller.
	//
	// jump is a *press* edge; pause and debug are *release* edges.
	//
	// THE KEYBOARD SLOT COMES BACK WITHOUT AN AIM, and that is the one thing
	// this class cannot finish. Mouse aim is the direction from the player's
	// centre *on screen* to the cursor, and where a player is on screen is a
	// camera and a viewport away from anything here. Level::apply_player_inputs
	// fills it, because that is what owns both. Until it does,
	// shoot_direction_requested is false, which is exactly what a pad slot with
	// a centred right stick says and needs no special case downstream.
	std::vector<PlayerInputData> player_inputs() const;

private:
	PlayerInputData pad_input(const labrador::Gamepads& gamepads,
		int slot) const;
	static PlayerInputData keyboard_and_mouse_input(
		const labrador::Keyboard& keyboard, const labrador::Mouse& mouse);

	const labrador::Gamepads* gamepads_ = nullptr;
	const labrador::Keyboard* keyboard_ = nullptr;
	const labrador::Mouse* mouse_ = nullptr;
	const GameTuning* tuning_ = nullptr;
};
