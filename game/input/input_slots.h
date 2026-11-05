#pragma once

#include "engine/input/gamepads.h"
#include <array>
#include <string>

// Stable device slots: gamepads first, then a shared keyboard/mouse slot.
// Input readers preserve disconnected slots with neutral entries. These are
// device indices, independent of the roster's maximum of four players.
namespace input_slots
{
	constexpr int pad_count = labrador::Gamepads::max_count;

	constexpr int keyboard_and_mouse = pad_count;

	constexpr int count = pad_count + 1;

	// One value for every pad slot, so a per-pad array follows pad_count.
	template <typename T>
	constexpr std::array<T, pad_count> per_pad(const T& value)
	{
		std::array<T, pad_count> result{};
		result.fill(value);
		return result;
	}

	constexpr bool is_pad(int slot)
	{
		return slot >= 0 && slot < pad_count;
	}

	constexpr bool is_keyboard_and_mouse(int slot)
	{
		return slot == keyboard_and_mouse;
	}

	// The base locale's menu_text.labels device copy, for probes, diagnostics
	// and InterfaceGameplay's input-slot overload. Player-facing text uses
	// MenuText::device_label, which a locale can reach.
	inline std::wstring device_name(int slot)
	{
		if (is_pad(slot))
		{
			return L"Pad " + std::to_wstring(slot + 1);
		}
		return is_keyboard_and_mouse(slot) ? L"Keyboard/mouse" : L"Waiting to join";
	}
}
