#pragma once

#include "game/objects/player_input_data.h"
#include <vector>

class Level;

// Replaces or observes the input each seat gets in one Level logic step,
// after the slot mapping and the keyboard/mouse aim are resolved, so what it
// sees and writes no longer depends on a device, a window or a camera.
// Seats are participant indices (0-based), including bots without a pane and
// idle human dummies without a device slot.
//
// Shipping bots run before the optional developer filter. Playback replaces
// their decisions entirely; recording observes the final participant inputs.
class SeatInputFilter
{
public:
	virtual ~SeatInputFilter() = default;

	// `seats` has one entry per participant, in roster order.
	virtual void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) = 0;
	virtual bool replaces_bot_inputs() const { return false; }

protected:
	SeatInputFilter() = default;
	SeatInputFilter(const SeatInputFilter&) = default;
	SeatInputFilter& operator=(const SeatInputFilter&) = default;
};
