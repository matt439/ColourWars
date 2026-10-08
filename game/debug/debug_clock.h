#pragma once

#include "game/objects/player_input_data.h"
#include <vector>

// Decides how many fixed-dt Level updates each engine step runs: the match is
// slowed, sped up, paused or stepped by updating it 0..max_runs times, never by
// scaling dt, so collisions, jump timing and ledge buffers keep their meaning.
class DebugClock
{
public:
	static constexpr int max_runs = 4;
	static constexpr float min_timescale = 0.25f;
	static constexpr float max_timescale = 4.0f;
	static constexpr int max_steps = 600;

	// Clamped to [min_timescale, max_timescale].
	void set_timescale(float scale);
	float timescale() const;

	void set_paused(bool paused);
	bool paused() const;

	// Pauses, and queues `count` single updates, one per engine step.
	void request_steps(int count);
	int pending_steps() const;

	// The number of updates this engine step. Paused: 1 while steps are
	// pending, else 0. Running: floor of the accumulated timescale, at most
	// max_runs.
	int updates_this_step();

	// Press edges seen on steps that ran no update are held and delivered on
	// the next step that does, so a jump tapped in slow motion is not lost.
	// Only jump_pressed and toggle_debug are latched: they are the only edges
	// Level reads (the pause key is handled by GameLevel before the clock).
	std::vector<PlayerInputData> latch(const std::vector<PlayerInputData>& inputs, int runs);

	// The inputs for the second and later runs of one engine step: the same
	// held state, with the latched edges cleared so no press applies twice.
	static std::vector<PlayerInputData> without_edges(std::vector<PlayerInputData> inputs);

	// After a rebuild: no partial step or old edge carries into the new level.
	void reset_accumulator();

	bool is_default() const;

private:
	float timescale_ = 1.0f;
	float accumulator_ = 0.0f;
	bool paused_ = false;
	int pending_steps_ = 0;
	std::vector<PlayerInputData> held_edges_;
};
