#include "game/pch.h"
#include "game/states/match_suspension.h"

bool MatchSuspension::covered() const { return this->covered_; }
bool MatchSuspension::pause_menu_open() const { return this->pause_menu_open_; }
bool MatchSuspension::counting_in() const { return this->countdown_.active(); }
ResumeCountdown& MatchSuspension::countdown() { return this->countdown_; }
const ResumeCountdown& MatchSuspension::countdown() const { return this->countdown_; }

bool MatchSuspension::audio_may_play(const SuspensionGate& gate) const
{
	return !this->covered_ && !this->pause_menu_open_ && !this->counting_in() &&
		gate.focused && !gate.clock_paused;
}

bool MatchSuspension::music_ducked() const
{
	return (this->pause_menu_open_ && this->covered_) ||
		(!this->covered_ && this->counting_in());
}

bool MatchSuspension::ducked_music_plays(const SuspensionGate& gate) const
{
	return this->music_ducked() && gate.focused && !gate.clock_paused;
}

bool MatchSuspension::rumble_paused(const SuspensionGate& gate) const
{
	return this->covered_ || this->pause_menu_open_ || this->counting_in() || gate.clock_paused;
}

bool MatchSuspension::may_run_unfocused(const SuspensionGate& gate) const
{
	return !this->covered_ && !this->pause_menu_open_ && gate.background_allowed;
}

bool MatchSuspension::suspends_level(const SuspensionGate& gate) const
{
	return this->covered_ || (!gate.focused && !gate.background_allowed);
}

bool MatchSuspension::live() const
{
	return !this->pause_menu_open_ && !this->counting_in();
}

void MatchSuspension::reset()
{
	this->background_continuing_ = false;
	this->countdown_.cancel();
	this->focus_interrupted_ = false;
}

void MatchSuspension::cover()
{
	this->background_continuing_ = false;
	this->covered_ = true;
}

void MatchSuspension::uncover() { this->covered_ = false; }
void MatchSuspension::open_pause_menu() { this->pause_menu_open_ = true; }
void MatchSuspension::close_pause_menu() { this->pause_menu_open_ = false; }

bool MatchSuspension::lose_focus(const SuspensionGate& gate, bool round_active)
{
	if (this->may_run_unfocused(gate))
	{
		this->background_continuing_ = true;
		this->focus_interrupted_ = false;
		return true;
	}
	this->background_continuing_ = false;
	this->focus_interrupted_ = round_active;
	return false;
}

void MatchSuspension::regain_focus() { this->background_continuing_ = false; }

bool MatchSuspension::take_focus_interruption()
{
	const bool interrupted = this->focus_interrupted_;
	this->focus_interrupted_ = false;
	return interrupted;
}

bool MatchSuspension::begin_count_in(bool round_active, float duration)
{
	this->focus_interrupted_ = false;
	if (!round_active) { return false; }
	this->countdown_.begin(duration);
	return true;
}

MatchSuspension::BackgroundChange MatchSuspension::background_changed(
	const SuspensionGate& gate, bool round_active)
{
	if (this->may_run_unfocused(gate))
	{
		// A previous ordinary focus interruption is already recorded; enabling
		// the policy releases that hold, never an existing count-in.
		this->background_continuing_ = true;
		this->focus_interrupted_ = false;
		return BackgroundChange::continuing;
	}
	if (this->background_continuing_)
	{
		this->background_continuing_ = false;
		this->focus_interrupted_ = round_active;
		return BackgroundChange::stopped;
	}
	return BackgroundChange::none;
}
