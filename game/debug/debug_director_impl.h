#pragma once

// Debug-only internals of DebugDirector. Included by game/debug/*.cpp and by
// the acceptance case, always under COLOURWARS_DEV_TOOLS.

#include "game/debug/debug_chord.h"
#include "game/debug/debug_clock.h"
#include "game/debug/debug_log.h"
#include "game/debug/debug_registry.h"
#include "game/debug/debug_targets.h"
#include "game/debug/debug_text_field.h"
#include "game/debug/match_plan.h"
#include <memory>
#include <optional>

class DebugMenuPage;
class GameData;
class GameTuning;
class MenuContext;
class MenuInput;
class MenuLevelSettings;
class Presentation;

namespace labrador
{
	class Application;
}

class DebugDirectorImpl
{
public:
	DebugDirectorImpl(GameTuning* tuning, Presentation* presentation);
	~DebugDirectorImpl();

	DebugDirectorImpl(const DebugDirectorImpl&) = delete;
	DebugDirectorImpl& operator=(const DebugDirectorImpl&) = delete;

	void attach(labrador::Application* application, GameData* data);

	DebugLog& log() { return this->log_; }
	DebugRegistry& registry() { return this->registry_; }
	const DebugRegistry& registry() const { return this->registry_; }
	DebugClock& clock() { return this->clock_; }
	const DebugClock& clock() const { return this->clock_; }
	DebugChord& chord() { return this->chord_; }
	DebugTextField& field() { return this->field_; }
	GameData* data() const { return this->data_; }

	// Runs a console line and logs its result.
	DebugStatus execute(std::string_view line);

	// Hooks, as described on DebugDirector.
	void run_launch_statements(const std::vector<std::string>& statements);
	void attach_match(DebugMatchTarget& match);
	void detach_match(const DebugMatchTarget& match);
	void before_level_build(DebugMatchTarget& match);
	void after_level_build(DebugMatchTarget& match);
	void filter_match_inputs(std::vector<PlayerInputData>& inputs) const;
	bool before_match_update(DebugMatchTarget& match, float dt);
	void update_match(DebugMatchTarget& match,
		const std::vector<PlayerInputData>& inputs, float dt);
	bool before_page_update(DebugPageTarget& page, float dt);

	// The console and the debug menu report their own lifetimes, so only one
	// of each is ever open.
	void set_console_open(bool open);
	bool console_open() const;
	void set_debug_menu(DebugMenuPage* page);
	bool debug_menu_open() const { return this->debug_menu_ != nullptr; }

	// The console calls this every frame it is open, so an F10 typed there is
	// not replayed as a debug-menu toggle once it closes.
	void forget_menu_key();

	// The debug menu's own input and context, made on first use: it is pushed
	// over any state, so it borrows none of theirs.
	MenuContext* menu_context();

	// Short "x0.25 PAUSED posed feedback-off GOD" text, empty when all default.
	std::string status_strip() const;

	// "paint  A 23.4%  B 11.0%  free 65.6%" and a per-seat line, or empty
	// when overlay.paint is off or no match is running.
	std::vector<std::string> paint_lines() const;

	// What `players`, `stage`, `visual` and `reload` read and queue.
	const MatchRequest& request() const { return this->request_; }
	bool in_match() const { return this->match_ != nullptr; }

	// Whether the debug menu's Apply would do anything: in a match, anything
	// queued; in the menus, a queued stage (the menus build nothing else).
	bool can_apply() const;

	// Clears the queued request, logging what was dropped and why.
	void drop_request(const std::string& reason);

	// Station names for `goto`: those of a concrete queued stage, else the
	// running match's. Never throws.
	std::vector<std::string> station_names() const;

	// Whether a pending screenshot waits for a queued change to apply first.
	bool screenshot_waits() const;

	// What `screenshot` stored, for the acceptance case.
	const std::optional<std::string>& pending_screenshot() const
	{
		return this->pending_screenshot_;
	}

private:
	GameTuning* tuning_ = nullptr;
	Presentation* presentation_ = nullptr;
	labrador::Application* application_ = nullptr;
	GameData* data_ = nullptr;

	DebugLog log_;
	DebugRegistry registry_;
	DebugClock clock_;
	DebugChord chord_;
	DebugTextField field_;

	DebugMatchTarget* match_ = nullptr;
	MatchRequest request_;
	bool leave_requested_ = false;
	bool console_open_ = false;
	bool waiting_notice_shown_ = false;

	DebugMenuPage* debug_menu_ = nullptr;
	std::unique_ptr<MenuInput> menu_input_;
	std::unique_ptr<MenuLevelSettings> menu_settings_;
	std::unique_ptr<MenuContext> menu_context_;

	// The thread message hook that turns F10 into a debug-menu toggle
	// instead of the system menu (see engine-reports.md, report 11).
	void* f10_hook_ = nullptr;

	// The visual a fresh match uses when the request names none: the last one
	// a debug plan applied.
	PlayerVisualMode remembered_visual_ = PlayerVisualMode::original;

	// Content validated by `reload`, assigned before the next level is built.
	std::unique_ptr<GameTuning> pending_tuning_;
	std::unique_ptr<Presentation> pending_presentation_;

	// Overlays and cheats live here, not in the level, so every build
	// re-applies them (after_level_build).
	bool overlay_player_ = false;
	bool overlay_collision_ = false;
	bool overlay_camera_ = false;
	bool overlay_paint_ = false;
	bool god_ = false;
	bool infinite_ammo_ = false;

	// A goto typed before a match exists, applied by the first build of a
	// match this director started (launch arguments, or `stage` in the menus),
	// or typed in a match with a rebuild queued, applied by that rebuild.
	struct PendingGoto
	{
		std::string station;
		int seat = 1;
	};
	std::optional<PendingGoto> pending_goto_;
	bool starting_from_plan_ = false;

	// A requested name ("" for the timestamped default), taken at the top of
	// the next update with nothing queued to change the picture first.
	std::optional<std::string> pending_screenshot_;

	void register_commands();
	void register_view_commands();
	void register_player_commands();
	bool open_console_if_requested();
	bool poll_debug_menu_toggle(float dt);
	void open_debug_menu();
	void run_binds(DebugMatchTarget& match);
	MatchPlanner planner() const;
	MatchPlanner::ConnectedPads connected_pads() const;
	void apply_pending_content();
	bool has_pending_content() const;

	// In a match: whether the next update rebuilds the level (a queued
	// request or reloaded content), which puts every player at a spawn.
	bool rebuild_queued() const;
	std::string queued_rebuild_description() const;

	// The visual the next plan would use: requested, current match, remembered.
	PlayerVisualMode planned_visual() const;
	void queue_visual(PlayerVisualMode mode);
	void log_match(const MatchPlan& plan, const std::string& how);

	void apply_overlays(bool player_text_changed);
	void apply_cheats();
	DebugStatus apply_goto(DebugMatchTarget& match, const std::string& station, int seat);

	// Takes a pending screenshot when nothing queued would change the picture
	// before the next frame.
	void capture_screenshot_if_ready();

	DebugStatus reload_content();
	std::vector<std::string> status_lines() const;
};
