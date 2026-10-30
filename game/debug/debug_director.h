#pragma once

#include "game/debug/dev_tools.h"
#include "game/debug/debug_targets.h"
#include "game/objects/opening_mode.h"
#include "game/objects/slot_input.h"
#include "game/objects/seat_input_filter.h"
#include "engine/input/keyboard.h"
#include <memory>
#include <string>
#include <vector>

class GameData;
class GameTuning;
class Presentation;
class DebugDirectorImpl;

// One step of GameLevel's match: the device slots its seat commands are built
// from, and the developer seat filter (bots, record, play) that sees and may
// replace each seat's input, null when nothing drives a seat (always, in Release).
struct MatchStep
{
	const std::vector<SlotInput>* inputs = nullptr;
	SeatInputFilter* seats = nullptr;
};

namespace labrador
{
	class Application;
	class Renderer;
}

// The developer debug service: console, debug menu, launch arguments, F-key
// binds, the debug clock, overlays, cheats, screenshots, queued match
// requests, live tuning, bots, seeded record and play, scripts, the live
// console and the perf overlay. The states call its hooks; Release keeps only
// the launch-argument warning.
//
// Its layout is the same in every configuration (one pointer). In Debug the
// pointer owns a DebugDirectorImpl; in Release it is always null and the hooks
// below are inline no-ops or pass-throughs (one match step a frame, with no
// developer filter), so no registry, console or command text is built.
class DebugDirector
{
public:
	// Borrows the live tuning and presentation, which `reload` replaces and
	// `feedback` edits. Both outlive Application, as this does.
	DebugDirector(GameTuning* tuning, Presentation* presentation);
	~DebugDirector();

	DebugDirector(const DebugDirector&) = delete;
	DebugDirector& operator=(const DebugDirector&) = delete;

	// A director with no implementation, for a GameData that was never given one.
	static DebugDirector& inert();

	bool enabled() const { return this->impl_ != nullptr; }

	// Called once the application and game data are complete.
	void attach(labrador::Application* application, GameData* data);

	// The process's arguments after the program name. Debug runs every
	// "+statement"; Release prints one stderr line when any are present.
	void run_launch_arguments(const std::vector<std::wstring>& arguments);

	// GameLevel::init calls this before building its first level, and the
	// destructor calls detach_match. Requests left over from the menus or from
	// an ended match (players/weapon stored without `stage`) are dropped here,
	// with a log line, so they never rebuild a match started another way.
	void attach_match(DebugMatchTarget& match);
	void detach_match(const DebugMatchTarget& match);

	// GameLevel's one build path calls this with no Level alive: it applies
	// content `reload` validated, and a restart from the pause or end menu
	// adopts whatever plan is queued (match.debug_adopt) and consumes a queued
	// restart, so the level is built once, with the queued settings.
	void before_level_build(DebugMatchTarget& match);

	// The same build path, once the new Level exists: re-applies overlays and
	// cheats, applies a goto stored before the match, and resets the clock's
	// partial step. Every build - first, menu restart, debug rebuild - runs it.
	void after_level_build(DebugMatchTarget& match);

	// GameLevel::update, on the inputs it just read: clears the pad Back and
	// Start release edges a debug-menu chord consumed (F1 overlay and pause).
	void filter_match_inputs(std::vector<SlotInput>& inputs);

	// The top of GameLevel::update. Opens the console or the debug menu, takes
	// a queued screenshot, runs the F-key binds and applies queued requests
	// (rebuild, leave). True: the match has done its work for this step.
	bool before_match_update(DebugMatchTarget& match, float dt, bool active = true);

	// GameLevel::update runs its own match step, in a loop these four hooks
	// bound; they decide only how many steps and with which developer filter.
	//
	// How many steps this frame runs, latching `inputs` for them: the debug
	// clock's 0..4 fixed-dt updates in Debug, one in Release.
	int match_steps(const std::vector<SlotInput>& inputs, float dt);
	// Before step `run`. False: the frame runs no more steps (a round that ends
	// mid fast-forward stops there). Otherwise sets what the step reads: the
	// latched inputs, their press edges on the first step only, and the seat
	// filter. Release leaves `step.inputs` as GameLevel set it.
	bool before_match_step(DebugMatchTarget& match, int run, MatchStep& step);
	// After the step. False: the frame runs no more steps (a replay reached its
	// last update, or a script waits for this one).
	bool after_match_step(DebugMatchTarget& match);
	// Once the frame's steps are done: a stepped update on the paused clock can
	// start the music or a weapon voice, so the audio is synced again.
	void end_match_steps(DebugMatchTarget& match, int runs);

	// Whether the debug clock is paused, so the match's voices stay paused.
	bool match_audio_paused() const;

	// Session-only Debug policy. Background scripts never imply window focus
	// or physical input, and hiding/minimising/power suspension remain gates.
	bool background_scripts() const;
	bool background_updates_allowed() const;
	// Mute both runtime sound groups before inactive work can create voices.
	// Restore the current preferences only on actual activation; no Save edit.
	void sync_background_audio(bool active);

	// GameLevel, right after each Level::suspend (a state pushed over the
	// match, focus lost, the paused clock's audio sync). Suspending clears
	// every player's jump buffer outside the input stream, so a recording
	// notes where it happened and a replay does it again there.
	void match_suspended(DebugMatchTarget& match);

	// Whether a bot or a replay drives the seat on this device slot, so a
	// missing pad there is no reason to pause the match. False in Release.
	bool drives_slot(int slot) const;

	// GameLevel's build, between the two build hooks: how the match it is
	// about to build begins. `chosen` is the game's own answer; `intro`
	// overrides it, and a recording or a replay always starts without one.
	OpeningMode opening_mode(OpeningMode chosen) const;

	// The top of every MenuPage::update. Opens the console or the debug menu,
	// takes a queued screenshot, starts a match queued by the console or by
	// launch arguments, and dismisses pages over a match a request is waiting
	// for. True: the page should not run its own logic this frame.
	bool before_page_update(DebugPageTarget& page, float dt, bool active = true);

	// A page owns raw keyboard input through its capture/release gate. Cancel
	// queued developer work on entry; drain reserved keys/chords each frame.
	void begin_keyboard_capture();
	void capture_input_frame(DebugPageTarget& page);
	void end_keyboard_capture();
	// A bare developer bind on a capturable Jump key is a warning, not a conflict.
	bool keyboard_jump_collision(labrador::Key key) const;

	// GameLevel::draw, last: the paint coverage overlay, and a small strip
	// naming non-default debug state (timescale, pause, feedback,
	// overlays, cheats) while the console is closed.
	void draw_status(labrador::Renderer& renderer) const;

	// GameLevel::draw, first: the perf overlay times the match draw from here
	// to draw_status.
	void begin_match_draw() const;
	// Current draw-to-draw CPU wall interval; unavailable is zero.
	double frame_time_ms() const;

	DebugDirectorImpl* impl() const { return this->impl_.get(); }

private:
	DebugDirector();
	std::unique_ptr<DebugDirectorImpl> impl_;
};

#if !COLOURWARS_DEV_TOOLS
inline void DebugDirector::attach(labrador::Application*, GameData*) {}
inline void DebugDirector::attach_match(DebugMatchTarget&) {}
inline void DebugDirector::detach_match(const DebugMatchTarget&) {}
inline void DebugDirector::before_level_build(DebugMatchTarget&) {}
inline void DebugDirector::after_level_build(DebugMatchTarget&) {}
inline void DebugDirector::filter_match_inputs(std::vector<SlotInput>&) {}
inline bool DebugDirector::before_match_update(DebugMatchTarget&, float, bool) { return false; }
inline int DebugDirector::match_steps(const std::vector<SlotInput>&, float) { return 1; }
inline bool DebugDirector::before_match_step(DebugMatchTarget&, int, MatchStep& step)
{
	step.seats = nullptr;
	return true;
}
inline bool DebugDirector::after_match_step(DebugMatchTarget&) { return true; }
inline void DebugDirector::end_match_steps(DebugMatchTarget&, int) {}
inline bool DebugDirector::match_audio_paused() const { return false; }
inline bool DebugDirector::background_scripts() const { return false; }
inline bool DebugDirector::background_updates_allowed() const { return false; }
inline void DebugDirector::sync_background_audio(bool) {}
inline void DebugDirector::match_suspended(DebugMatchTarget&) {}
inline bool DebugDirector::drives_slot(int) const { return false; }
inline OpeningMode DebugDirector::opening_mode(OpeningMode chosen) const { return chosen; }
inline bool DebugDirector::before_page_update(DebugPageTarget&, float, bool) { return false; }
inline void DebugDirector::begin_keyboard_capture() {}
inline void DebugDirector::capture_input_frame(DebugPageTarget&) {}
inline void DebugDirector::end_keyboard_capture() {}
inline bool DebugDirector::keyboard_jump_collision(labrador::Key) const { return false; }
inline void DebugDirector::draw_status(labrador::Renderer&) const {}
inline void DebugDirector::begin_match_draw() const {}
inline double DebugDirector::frame_time_ms() const { return 0.0; }
#endif
