#pragma once

#include "game/debug/dev_tools.h"
#include "game/debug/debug_targets.h"
#include "game/objects/opening_mode.h"
#include "engine/input/keyboard.h"
#include <memory>
#include <string>
#include <vector>

class GameData;
class GameTuning;
class Presentation;
class DebugDirectorImpl;

namespace labrador
{
	class Application;
	class Renderer;
}

// The developer debug service: console, debug menu, launch arguments, F-key
// binds, the debug clock, overlays, cheats, screenshots, queued match
// requests, live tuning, bots, seeded record and play, scripts, the live
// console and the perf overlay. The states call
// its hooks; it does nothing in Release.
//
// Its layout is the same in every configuration (one pointer). In Debug the
// pointer owns a DebugDirectorImpl; in Release it is always null and the hooks
// below are inline no-ops, so no registry, console or command text is built.
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
	void filter_match_inputs(std::vector<PlayerInputData>& inputs) const;

	// The top of GameLevel::update. Opens the console or the debug menu, takes
	// a queued screenshot, runs the F-key binds and applies queued requests
	// (rebuild, leave). True: the match has done its work for this step.
	bool before_match_update(DebugMatchTarget& match, float dt);

	// Replaces GameLevel's single Level::update with the debug clock's
	// 0..4 fixed-dt updates.
	void update_match(DebugMatchTarget& match,
		const std::vector<PlayerInputData>& inputs, float dt);

	// Whether the debug clock is paused, so the match's voices stay paused.
	bool match_audio_paused() const;

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
	bool before_page_update(DebugPageTarget& page, float dt);

	// A page owns raw keyboard input through its capture/release gate. Cancel
	// queued developer work on entry; drain reserved keys/chords each frame.
	void begin_keyboard_capture();
	void capture_input_frame(DebugPageTarget& page);
	bool capture_blocked_key_down() const;
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
inline void DebugDirector::filter_match_inputs(std::vector<PlayerInputData>&) const {}
inline bool DebugDirector::before_match_update(DebugMatchTarget&, float) { return false; }
inline void DebugDirector::update_match(DebugMatchTarget& match,
	const std::vector<PlayerInputData>& inputs, float dt)
{
	match.debug_update_level(inputs, dt, nullptr);
}
inline bool DebugDirector::match_audio_paused() const { return false; }
inline void DebugDirector::match_suspended(DebugMatchTarget&) {}
inline bool DebugDirector::drives_slot(int) const { return false; }
inline OpeningMode DebugDirector::opening_mode(OpeningMode chosen) const { return chosen; }
inline bool DebugDirector::before_page_update(DebugPageTarget&, float) { return false; }
inline void DebugDirector::begin_keyboard_capture() {}
inline void DebugDirector::capture_input_frame(DebugPageTarget&) {}
inline bool DebugDirector::capture_blocked_key_down() const { return false; }
inline void DebugDirector::end_keyboard_capture() {}
inline bool DebugDirector::keyboard_jump_collision(labrador::Key) const { return false; }
inline void DebugDirector::draw_status(labrador::Renderer&) const {}
inline void DebugDirector::begin_match_draw() const {}
inline double DebugDirector::frame_time_ms() const { return 0.0; }
#endif
