#pragma once

#include "game/input_slots.h"
#include "game/objects/slot_input.h"
#include <array>
#include <vector>

// A real-time gate around Level updates. The owner samples and filters input
// before checking pause, then advances this clock instead of the match clock.
class ResumeCountdown
{
public:
	void begin(float duration);
	void cancel();
	bool active() const;
	float remaining() const;
	// True once, on completion. Excess time never spills into the match.
	bool advance(float dt);

	// Movement and aim remain deliberate inputs. Actions held over the final
	// count need a release before another press/shot/drop; a fresh pause may
	// interrupt the count. Call once per sampled tick, before advance().
	void filter_inputs(std::vector<SlotInput>& inputs);

private:
	struct HeldActions
	{
		bool jump = false;
		bool fire = false;
		bool drop = false;
		bool pause = false;
	};
	double remaining_ = 0.0;
	std::array<HeldActions, input_slots::count> held_{};
};
