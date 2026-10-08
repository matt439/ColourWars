#pragma once

#include "engine/input/gamepads.h"
#include <string>

// Stable device slots: gamepads first, then a shared keyboard/mouse slot.
// Input readers preserve disconnected slots with neutral entries. These are
// device indices, independent of the roster's maximum of four players.
namespace input_slots
{
	constexpr int pad_count = labrador::Gamepads::max_count;

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

	inline std::wstring device_name(int slot)
	{
		if (is_pad(slot))
		{
			return L"Pad " + std::to_wstring(slot + 1);
		}
		return is_keyboard_and_mouse(slot) ? L"Keyboard/mouse" : L"Waiting to join";
	}
}
