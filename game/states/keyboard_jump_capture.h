#pragma once

#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "game/input_bindings.h"
#include <optional>

// One physical keyboard gesture for one keyboard action. The page samples once
// per update; queries and repeated samples of a terminal frame never accept the
// choice a second time. All named keys participate in neutral/chord detection,
// including keys outside the small binding domain. Shoot's gesture also counts
// mouse buttons; every other action ignores the mouse.
class KeyboardJumpCapture
{
public:
	enum class Phase { waiting_neutral, ready, holding, cancelling, complete, cancelled };
	enum class Rejection { none, unsupported, ambiguous, conflict };

	// Restarts the gesture for `action`, refusing controls another action of
	// `pending` already uses. The plain reset keeps the current target, which
	// starts as Jump against the default choices.
	void reset(const InputBindings& pending, KeyboardAction action);
	void reset();
	void focus_lost();
	void update(const labrador::KeyboardState& now,
		const labrador::KeyboardState& before, bool cancel_requested = false);
	void update(const labrador::KeyboardState& now, const labrador::KeyboardState& before,
		const labrador::MouseState& mouse_now, const labrador::MouseState& mouse_before,
		bool cancel_requested = false);

	Phase phase() const { return this->phase_; }
	Rejection rejection() const { return this->rejection_; }
	KeyboardAction action() const { return this->action_; }
	bool accepts_mouse() const { return this->action_ == KeyboardAction::shoot; }
	// The key part of the candidate/result; none for a mouse button.
	labrador::Key candidate() const { return this->candidate_.key; }
	std::optional<labrador::Key> result() const;
	const DesktopControl& candidate_control() const { return this->candidate_; }
	std::optional<DesktopControl> control_result() const { return this->result_; }
	bool focus_cancelled() const { return this->focus_cancelled_; }
private:
	void reject(Rejection reason);
	InputBindings pending_;
	KeyboardAction action_ = KeyboardAction::jump;
	Phase phase_ = Phase::waiting_neutral;
	Rejection rejection_ = Rejection::none;
	DesktopControl candidate_;
	std::optional<DesktopControl> result_;
	bool focus_cancelled_ = false;
};
