#pragma once

#include "game/input_slots.h"
#include <array>

// The pad chord that opens and closes the debug menu: hold Back and Start
// together for hold_seconds. Pure, so the acceptance case drives it frame by
// frame without a pad.
//
// A session starts on the first frame a pad has both buttons down and ends on
// the first frame it has neither (or disconnects). The frame a session starts
// adds its dt; the chord fires once per session, on the frame the held time
// reaches hold_seconds. A lone Back or Start never starts one.
//
// While a session is active, and on the frame it ends, the pad's Back and
// Start edges belong to the chord: suppressing() says so, and the director
// clears them from match input (F1 overlay, pause) and resets menu arming
// (pause). That holds for an aborted chord too, so a deliberate attempt that
// is let go early neither pauses nor toggles the per-player overlay.
class DebugChord
{
public:
	static constexpr float hold_seconds = 0.5f;

	struct Pad
	{
		bool connected = false;
		bool back = false;
		bool start = false;
	};
	using Pads = std::array<Pad, input_slots::pad_count>;

	// One call per frame, before anything reads pad edges. True on the frame
	// a session's hold reaches hold_seconds.
	bool update(const Pads& pads, float dt);

	bool suppressing(int slot) const;
	bool any_suppressing() const;

	void reset();

private:
	struct Session
	{
		bool active = false;
		bool was_active = false;
		bool fired = false;
		float held = 0.0f;
	};
	std::array<Session, input_slots::pad_count> sessions_{};
};
