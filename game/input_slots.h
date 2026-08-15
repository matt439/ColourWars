#pragma once

#include "engine/input/gamepads.h"

// Which input a slot number means.
//
// Every list of inputs in this game is indexed by slot and always has been -
// PlayerInput and MenuInput both answer one entry per slot, absent inputs
// carrying a neutral entry rather than being omitted, because the index is
// what a player is bound to (MenuLevelSettings::set_player_num) and a list
// that skipped the empty ones would hand a player somebody else's controller.
//
// The keyboard and mouse are one more slot on the end of that list. They are
// one slot and not two: there is one of each, on one desk, in front of one
// person, and a player aiming with the mouse is the same player walking with
// the keys. Adding them here rather than as a parallel path is what makes them
// an alternative to a pad rather than a special case - every page that walks
// slots, every roster that binds one to a player, and Level::apply_player_inputs
// itself all pick this up without knowing it happened.
//
// AN INPUT SLOT IS NOT A PLAYER. There are five ways to drive a character and
// still at most four characters, because the game is two teams of two on one
// screen. Nothing here raises the player cap, and level_loaded_info.cpp still
// asks a stage for Gamepads::max_count spawns per team rather than this
// number - a fifth spawn would be a place for a fifth player to stand, and
// there is no fifth player.
namespace input_slots
{
	// The pads, first and unchanged, so a slot number that meant pad 2 before
	// the keyboard existed still means pad 2.
	constexpr int pad_count = labrador::Gamepads::max_count;

	// The keyboard and mouse together, after them.
	constexpr int keyboard_and_mouse = pad_count;

	constexpr int count = pad_count + 1;

	constexpr bool is_pad(int slot)
	{
		return slot >= 0 && slot < pad_count;
	}

	constexpr bool is_keyboard_and_mouse(int slot)
	{
		return slot == keyboard_and_mouse;
	}
}
