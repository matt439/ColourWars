#pragma once

#include "engine/math/rectanglef.h"
#include "game/objects/level_overlays.h"
#include "game/objects/level_station.h"
#include "game/objects/player_input_data.h"
#include "game/objects/seat_input_filter.h"
#include "game/states/menu_level_settings.h"
#include <string>
#include <vector>

class Level;

// Paint coverage as the developer overlay shows it: areas in world units,
// seats in view order (an idle dummy paints nothing).
struct MatchPaint
{
	float team_a = 0.0f;
	float team_b = 0.0f;
	float total = 0.0f;
	std::vector<float> seats;
};

// What the debug director may ask of a running match. GameLevel implements
// it privately; the director only reaches it through its own hooks, which
// GameLevel calls at the top of its update, when no Level::update is on the
// call stack, or from a console or debug-menu state above it.
class DebugMatchTarget
{
public:
	virtual ~DebugMatchTarget() = default;

	virtual const MenuLevelSettings& debug_settings() const = 0;

	// Builds a fresh level from `plan` through the match's one build path,
	// which calls DebugDirector::before_level_build between destroying the old
	// level and building the new one. reset_session starts a new session.
	virtual void debug_rebuild(const MenuLevelSettings& plan, bool reset_session) = 0;

	// Inside a build already in progress (a restart from the pause or end
	// menu): take `plan` as the settings that build uses, without building.
	virtual void debug_adopt(const MenuLevelSettings& plan, bool reset_session) = 0;

	// Leaves the match for the main menu's home page.
	virtual void debug_leave() = 0;

	// One Level::update. `seats` (bots, record, play) sees and may replace
	// each seat's input for that update's logic step; null when nothing drives
	// a seat, and always null in Release.
	virtual void debug_update_level(const std::vector<PlayerInputData>& inputs,
		float dt, SeatInputFilter* seats) = 0;
	virtual bool debug_level_finished() const = 0;

	// The running level, read-only, for bots and match fingerprints. Never
	// null while the match is attached.
	virtual const Level* debug_level() const = 0;

	// Pauses the level's voices while the debug clock is paused, the match is
	// covered or the window is inactive; resumes them otherwise.
	virtual void debug_sync_audio() = 0;

	// One line per view: device, position, move state.
	virtual std::vector<std::string> debug_status() const = 0;

	// Immediate edits of the live level. Seats are views (0-based).
	virtual void debug_set_overlays(const LevelOverlays& overlays) = 0;
	// Every player's debug text on or off, overwriting each player's own
	// F1 / pad Back toggle: pushed only when overlay.player is set.
	virtual void debug_set_player_text(bool showing) = 0;
	virtual void debug_set_cheats(bool damage_immune, bool infinite_ammo) = 0;
	// Authored stations, then spawn_a and spawn_b.
	virtual std::vector<LevelStation> debug_stations() const = 0;
	// False: no such seat.
	virtual bool debug_place(int seat, const mattmath::Vector2F& top_left) = 0;
	virtual bool debug_respawn(int seat) = 0;
	virtual MatchPaint debug_paint() const = 0;

	// Phase 4 ----------------------------------------------------------------

	// The countdown is over and the players move (`wait match`).
	virtual bool debug_countdown_over() const = 0;
	// What each seat's HUD says instead of its device ("Bot: paint",
	// "Replay"), by view; an empty string or a short list keeps the device.
	virtual void debug_set_seat_labels(const std::vector<std::wstring>& labels) = 0;
	// Window-pixel rectangles the debug panels must not cover: the opening
	// cards during the countdown.
	virtual std::vector<mattmath::RectangleF> debug_screen_obstacles() const = 0;

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
	virtual bool debug_start_match(const MenuLevelSettings& plan) = 0;

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
