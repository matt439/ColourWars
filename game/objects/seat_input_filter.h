#pragma once

#include "game/objects/player_input_data.h"
#include <vector>

class Level;

// Replaces or observes the input each seat gets in one Level logic step,
// after the slot mapping and the keyboard/mouse aim are resolved, so what it
// sees and writes no longer depends on a device, a window or a camera.
// Seats are views (0-based), which is how an idle dummy (no device slot) is
// addressed at all.
//
// Present in every build, implemented only by the Debug developer tools
// (bots, record and play); nothing in Release derives from it, so it emits
// no vtable or type information there. Level::update takes one as a
// parameter, never as a member.
class SeatInputFilter
{
public:
	virtual ~SeatInputFilter() = default;

	// `seats` has one entry per view, in view order.
	virtual void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) = 0;

protected:
	SeatInputFilter() = default;
	SeatInputFilter(const SeatInputFilter&) = default;
	SeatInputFilter& operator=(const SeatInputFilter&) = default;
};
