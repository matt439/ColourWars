#pragma once

#include "engine/math/rectanglef.h"
#include "game/objects/level_overlays.h"
#include "game/objects/level_station.h"
#include "game/match_setup.h"
#include "game/round_tape.h"
#include <string>
#include <vector>

class Level;

// Paint coverage as the developer overlay shows it: areas in world units,
// seats in view order, including device-less players driven by bots.
struct MatchPaint
{
	float team_a = 0.0f;
	float team_b = 0.0f;
	float total = 0.0f;
	std::vector<float> seats;
};

// What the debug director may ask of a running match, split by the client
// that asks. GameLevel implements all three privately, through
// DebugMatchTarget; the director only reaches them through its own hooks,
// which GameLevel calls at the top of its update, around each match step,
// when no Level::update is on the call stack, or from a console or debug-menu
// state above it. The match step itself is GameLevel's own and is not here.

// Reading the match: bots, fingerprints, the status and paint readouts,
// goto's stations, script waits and the panels' placement. Never changes it.
class DebugMatchView
{
public:
	virtual ~DebugMatchView() = default;

	virtual const MatchSetup& debug_settings() const = 0;
	virtual bool debug_level_finished() const = 0;

	// The running level, read-only, for bots and match fingerprints. Never
	// null while the match is attached.
	virtual const Level* debug_level() const = 0;

	// One line per view: device, position, move state.
	virtual std::vector<std::string> debug_status() const = 0;
	// Authored stations, then spawn_a and spawn_b.
	virtual std::vector<LevelStation> debug_stations() const = 0;
	virtual MatchPaint debug_paint() const = 0;

	// The countdown is over and the players move (`wait match`).
	virtual bool debug_countdown_over() const = 0;
	// A pacing capture needs live simulation, including the screen's resume
	// gate. Kept separate from `wait match`, which describes the round start.
	virtual bool debug_perf_ready() const
	{
		return this->debug_countdown_over() && !this->debug_level_finished();
	}
	// Window-pixel rectangles the debug panels must not cover: the opening
	// cards during the countdown.
	virtual std::vector<mattmath::RectangleF> debug_screen_obstacles() const = 0;

protected:
	DebugMatchView() = default;
	DebugMatchView(const DebugMatchView&) = default;
	DebugMatchView& operator=(const DebugMatchView&) = default;
};

// Driving the match as a whole: the queued requests' rebuilds and leave, and
// the clock's and background policy's audio.
class DebugMatchControl
{
public:
	virtual ~DebugMatchControl() = default;

	// Builds a fresh level from `plan` through the match's one build path,
	// which calls DebugDirector::before_level_build between destroying the old
	// level and building the new one. reset_session starts a new session.
	virtual void debug_rebuild(const MatchSetup& plan, bool reset_session) = 0;

	// Inside a build already in progress (a restart from the pause or end
	// menu): take `plan` as the settings that build uses, without building.
	virtual void debug_adopt(const MatchSetup& plan, bool reset_session) = 0;

	// Leaves the match for the main menu's home page.
	virtual void debug_leave() = 0;

	// Pauses the level's voices while the debug clock is paused, the match is
	// covered or the window is inactive; resumes them otherwise.
	virtual void debug_sync_audio() = 0;
	// Applies one recorded gap to the level (RoundTape::apply_gap: a suspend
	// clears every jump buffer, a resume re-reads the held jumps), with none
	// of the screen's bookkeeping and no voice paused: playback repeats each
	// where the recording had it.
	virtual void debug_apply_gap(const RoundTape::Gap& gap) = 0;
	// A session policy/native hard gate changed while focus may already be
	// absent. Real GameLevel reconciles immediately; other targets need none.
	virtual void debug_background_changed() {}

protected:
	DebugMatchControl() = default;
	DebugMatchControl(const DebugMatchControl&) = default;
	DebugMatchControl& operator=(const DebugMatchControl&) = default;
};

// Immediate edits of the live level: the overlay, cheat and player commands
// and the HUD's seat labels. Seats are views (0-based).
class DebugMatchEdits
{
public:
	virtual ~DebugMatchEdits() = default;

	virtual void debug_set_overlays(const LevelOverlays& overlays) = 0;
	// Every player's debug text on or off, overwriting each player's own
	// F1 / pad Back toggle: pushed only when overlay.player is set.
	virtual void debug_set_player_text(bool showing) = 0;
	virtual void debug_set_cheats(bool damage_immune, bool infinite_ammo) = 0;
	// False: no such seat.
	virtual bool debug_place(int seat, const mattmath::Vector2F& top_left) = 0;
	virtual bool debug_respawn(int seat) = 0;
	virtual bool debug_kill(int seat) = 0;
	// What each seat's HUD says instead of its device ("Bot: paint",
	// "Replay"), by view; an empty string or a short list keeps the device.
	virtual void debug_set_seat_labels(const std::vector<std::wstring>& labels) = 0;

protected:
	DebugMatchEdits() = default;
	DebugMatchEdits(const DebugMatchEdits&) = default;
	DebugMatchEdits& operator=(const DebugMatchEdits&) = default;
};

// The whole of what the director holds while a match is attached.
class DebugMatchTarget : public DebugMatchView, public DebugMatchControl, public DebugMatchEdits
{
protected:
	DebugMatchTarget() = default;
	DebugMatchTarget(const DebugMatchTarget&) = default;
	DebugMatchTarget& operator=(const DebugMatchTarget&) = default;
};

// What the debug director may ask of the menu page on top. MenuPage
// implements it, so the director needs no MenuPage and the acceptance case
// can drive the unwinding with a fake page.
class DebugPageTarget
{
public:
	virtual ~DebugPageTarget() = default;

	// May this page be dismissed so a queued match starts with `plan`? Only
	// main-menu pages, whose frame starts a match on close, say yes.
	virtual bool debug_start_match(const MatchSetup& plan) = 0;

	// Close this page the way its own controls would, so a queued request
	// can reach the match (or the menus) beneath it. `leaving_match` asks for
	// the way out of the match rather than back into it. False: the page has
	// no such exit and the request waits.
	virtual bool debug_dismiss(bool leaving_match) = 0;

	// Forget the armed menu presses of one pad slot: a chord is in progress
	// on it. The keyboard and other pads keep theirs.
	virtual void debug_reset_input(int pad_slot) = 0;

protected:
	DebugPageTarget() = default;
	DebugPageTarget(const DebugPageTarget&) = default;
	DebugPageTarget& operator=(const DebugPageTarget&) = default;
};
