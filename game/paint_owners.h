#pragma once

#include "game/input_slots.h"

// Humans retain their stable input-slot credit after pad reassignment. Bots
// and device-less Debug players get separate credit by roster participant;
// these owner indices never enter the input readers.
namespace paint_owners
{
	constexpr int dummy_count = 4;
	constexpr int count = input_slots::count + dummy_count;

	constexpr int index(int device_slot, int view)
	{
		if (device_slot >= 0 && device_slot < input_slots::count)
		{
			return device_slot;
		}
		return device_slot == -1 && view >= 0 && view < dummy_count ?
			input_slots::count + view : -1;
	}
}
