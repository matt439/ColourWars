#pragma once

#include "game/states/resume_countdown.h"

// What the window and the debug director say about a match, read once per
// GameLevel callback and again only after a developer hook that may change it.
struct SuspensionGate
{
	// The window has focus (StateContext::active).
	bool focused = true;
	// The debug clock is paused, so the match's voices stay paused. Always
	// false in Release.
	bool clock_paused = false;
	// Debug background scripts may run the match while it is unfocused.
	// Always false in Release.
	bool background_allowed = false;
};

// Every reason a live round is suspended, and the answers GameLevel acts on.
// Stack coverage, the pause menu, a resume count-in and focus are independent
// reasons: losing one never ends another, and audio plays only when none
// holds. A focus loss over live play is remembered, so regaining focus (or
// uncovering) counts in instead of resuming at once; Debug background mode
// replaces that hold while it runs the match unfocused.
//
// It decides and records; GameLevel does the audio, rumble and Level calls,
// in the order its callbacks always have.
class MatchSuspension
{
public:
	// The reasons.
	bool covered() const;
	bool pause_menu_open() const;
	bool counting_in() const;
	// The count-in clock. GameLevel filters and advances it each update.
	ResumeCountdown& countdown();
	const ResumeCountdown& countdown() const;

	// Gameplay voices: nothing covers the match, no pause or count-in, the
	// window is focused and the debug clock runs.
	bool audio_may_play(const SuspensionGate& gate) const;
	// The arena music plays ducked: the pause menu is the cover, or an
	// uncovered count-in is running.
	bool music_ducked() const;
	// Ducked music is also audible: focused and the debug clock running.
	bool ducked_music_plays(const SuspensionGate& gate) const;
	// Motors hold still: covered, paused, counting in or the debug clock paused.
	bool rumble_paused(const SuspensionGate& gate) const;
	// Debug background mode may run the uncovered, unpaused match unfocused.
	bool may_run_unfocused(const SuspensionGate& gate) const;
	// The level must be suspended, not just its voices paused: covered, or
	// unfocused without background permission.
	bool suspends_level(const SuspensionGate& gate) const;
	// No pause or count-in stands between the round and its next step.
	bool live() const;

	// A new level: no count-in, no focus hold, no background run.
	void reset();
	// A state was pushed over the match / popped off it.
	void cover();
	void uncover();
	void open_pause_menu();
	void close_pause_menu();

	// Focus was lost. True when Debug background mode keeps the match running;
	// otherwise a round that was `round_active` must count in on return.
	bool lose_focus(const SuspensionGate& gate, bool round_active);
	// Focus came back. Background running ends; any focus hold stays for
	// take_focus_interruption to answer once the match is uncovered.
	void regain_focus();
	// Whether a focus loss is still owed a count-in, clearing it.
	bool take_focus_interruption();
	// Begins the count-in for a round that is `round_active`, clearing any
	// focus hold either way. True when the clock started.
	bool begin_count_in(bool round_active, float duration);

	// The Debug background policy changed while unfocused.
	enum class BackgroundChange { none, continuing, stopped };
	BackgroundChange background_changed(const SuspensionGate& gate, bool round_active);

private:
	bool covered_ = false;
	bool pause_menu_open_ = false;
	bool focus_interrupted_ = false;
	bool background_continuing_ = false;
	ResumeCountdown countdown_;
};
