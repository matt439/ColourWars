#pragma once

#include "game/objects/player_input_data.h"
#include "game/objects/seat_input_filter.h"
#include "game/match_setup.h"
#include <cstddef>
#include <vector>

class Level;

// One round, kept so it can be built again and any stretch of it watched
// (ROAD#19, the end-of-round instant replay): the setup and the seed rand()
// had when the level was built, every participant's final input on every
// logic step, and
// the two things between steps that change a player - a suspend clears every
// jump buffer, and a resume re-reads which jumps are still held.
//
// The same contract as the Debug recordings (DBG#2): it holds on this build,
// this machine and this step, within one round, because the simulation is
// deterministic from the build. It never leaves memory, so that is all it
// needs. Debug place/respawn/kill spoil the tape; a cheat change after the first
// recorded step also spoils it. Live tuning edits and suspends injected by Debug
// playback are not observed; such a replay can diverge (docs/feature-roadmap.md, ROAD#19).
class RoundTape
{
public:
	// A fresh, empty tape for a level built from `settings` after std::srand(seed).
	// The setup is copied: the End menu's re-pick pages may change the match's
	// own before its Watch Replay is chosen.
	void start(unsigned int seed, const MatchSetup& settings);
	void clear();
	void spoil();
	// The Debug cheats the level plays under. A change once the round has
	// stepped spoils the tape, since the steps before it ran without.
	void set_cheats(bool damage_immune, bool infinite_ammo);

	// Recorded and not spoiled.
	bool playable() const;
	unsigned int seed() const { return this->seed_; }
	const MatchSetup& settings() const { return this->settings_; }
	bool damage_immune() const { return this->damage_immune_; }
	bool infinite_ammo() const { return this->infinite_ammo_; }

	int steps() const;
	const std::vector<PlayerInputData>& seats(int step) const;
	float dt(int step) const;
	// The first step of the last `seconds` of the tape, or 0.
	int step_before_end(float seconds) const;

	// Recording. `seats` is one logic step's final inputs, in roster order.
	void record_step(const std::vector<PlayerInputData>& seats, float dt);
	void record_suspend();
	// The seat commands Level::prepare_resume read, in roster order.
	void record_resume(const std::vector<PlayerInputData>& seats);

	// Playback: what happened to the level between step - 1 and `step`.
	void apply_gaps(Level& level, int step) const;

private:
	struct Gap
	{
		int step = 0;
		bool resume = false;
		std::vector<PlayerInputData> seats;
	};

	unsigned int seed_ = 0;
	MatchSetup settings_;
	bool damage_immune_ = false;
	bool infinite_ammo_ = false;
	bool started_ = false;
	bool spoiled_ = false;
	std::vector<std::vector<PlayerInputData>> seats_;
	std::vector<float> dts_;
	std::vector<Gap> gaps_;
};

// Records the final input of every logic step into a tape, after `next` (the
// Debug bots, record or play, or nothing) has had its say. Observes only.
class RoundTapeRecorder final : public SeatInputFilter
{
public:
	explicit RoundTapeRecorder(RoundTape* tape) : tape_(tape) {}

	// Before each Level::update: the filter that follows this one, and the
	// step's dt, which filter_seats does not see.
	void prepare(SeatInputFilter* next, float dt);

	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;
	bool replaces_bot_inputs() const override;

private:
	RoundTape* tape_ = nullptr;
	SeatInputFilter* next_ = nullptr;
	float dt_ = 0.0f;
};

// Replaces every seat's input from a tape, one step per logic step, from
// `first`. The level's own bots do not run.
class RoundTapePlayer final : public SeatInputFilter
{
public:
	RoundTapePlayer(const RoundTape* tape, int first) : tape_(tape), step_(first) {}

	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;
	bool replaces_bot_inputs() const override { return true; }

	// The next step filter_seats will supply.
	int step() const { return this->step_; }
	bool finished() const;

private:
	const RoundTape* tape_ = nullptr;
	int step_ = 0;
};
