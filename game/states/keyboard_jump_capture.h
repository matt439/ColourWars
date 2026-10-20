#pragma once

#include "engine/input/keyboard.h"
#include <optional>

// One physical keyboard gesture. The page samples once per update; queries and
// repeated samples of a terminal frame never accept the choice a second time.
// All named keys participate in neutral/chord detection, including keys outside
// the small binding domain. A debug hook can supply its intercepted F10 state.
class KeyboardJumpCapture
{
public:
	enum class Phase { waiting_neutral, ready, holding, cancelling, complete, cancelled };
	enum class Rejection { none, unsupported, ambiguous, conflict };

	void reset();
	void focus_lost();
	void update(const labrador::KeyboardState& now,
		const labrador::KeyboardState& before, bool blocked_key_down = false,
		bool cancel_requested = false);

	Phase phase() const { return this->phase_; }
	Rejection rejection() const { return this->rejection_; }
	labrador::Key candidate() const { return this->candidate_; }
	std::optional<labrador::Key> result() const { return this->result_; }
	bool focus_cancelled() const { return this->focus_cancelled_; }
private:
	void reject(Rejection reason);
	Phase phase_ = Phase::waiting_neutral;
	Rejection rejection_ = Rejection::none;
	labrador::Key candidate_ = labrador::Key::none;
	std::optional<labrador::Key> result_;
	bool focus_cancelled_ = false;
};
