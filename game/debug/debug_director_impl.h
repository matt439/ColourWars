#pragma once

// Debug-only internals of DebugDirector. Included by game/debug/*.cpp and by
// the acceptance case, always under COLOURWARS_DEV_TOOLS.

#include "game/objects/opening_mode.h"
#include "game/debug/debug_binds.h"
#include "game/debug/debug_background.h"
#include "game/debug/debug_chord.h"
#include "game/debug/debug_clock.h"
#include "game/debug/debug_log.h"
#include "game/debug/debug_perf.h"
#include "game/debug/debug_registry.h"
#include "game/debug/debug_script_runner.h"
#include "game/debug/debug_targets.h"
#include "game/debug/debug_text_field.h"
#include "game/debug/match_plan.h"
#include "game/debug/match_fingerprint.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <random>

class BotDriver;
class DebugConsoleView;
class DebugMenuPage;
class ReplayFile;
class ReplayPlayer;
class ReplayRecorder;
class SeatInputFilter;
class TuneDocument;
class TuneTarget;
class GameData;
class GameTuning;
class MenuContext;
class MenuInput;
class MenuLevelSettings;
class Presentation;

namespace labrador
{
	class Application;
	class Renderer;
}

class DebugDirectorImpl
{
public:
	DebugDirectorImpl(GameTuning* tuning, Presentation* presentation);
	~DebugDirectorImpl();

	DebugDirectorImpl(const DebugDirectorImpl&) = delete;
	DebugDirectorImpl& operator=(const DebugDirectorImpl&) = delete;

	void attach(labrador::Application* application, GameData* data);
	bool background_scripts() const { return this->background_scripts_; }
	bool background_updates_allowed() const;
	void sync_background_audio(bool active);
	// CPU acceptance supplies facts without changing the real focus/presence
	// state. Empty restores the native observer; no probe means unavailable.
	void set_background_probe(std::function<DebugBackgroundState()> probe);

	DebugLog& log() { return this->log_; }
	const DebugLog& log() const { return this->log_; }
	DebugRegistry& registry() { return this->registry_; }
	const DebugRegistry& registry() const { return this->registry_; }
	DebugClock& clock() { return this->clock_; }
	const DebugClock& clock() const { return this->clock_; }
	DebugChord& chord() { return this->chord_; }
	DebugTextField& field() { return this->field_; }
	const DebugTextField& field() const { return this->field_; }
	GameData* data() const { return this->data_; }

	// Runs a console line and logs its result.
	DebugStatus execute(std::string_view line);

	// What Enter in either console does: echo "] line", run it, and keep the
	// history file (Debug, with a user directory) up to date.
	DebugStatus submit_console_line(const std::string& line);

	// Hooks, as described on DebugDirector.
	void run_launch_statements(const std::vector<std::string>& statements);
	void attach_match(DebugMatchTarget& match);
	void detach_match(const DebugMatchTarget& match);
	void before_level_build(DebugMatchTarget& match);
	void after_level_build(DebugMatchTarget& match);
	void filter_match_inputs(std::vector<PlayerInputData>& inputs) const;
	bool before_match_update(DebugMatchTarget& match, float dt, bool active = true);
	void update_match(DebugMatchTarget& match,
		const std::vector<PlayerInputData>& inputs, float dt);
	bool before_page_update(DebugPageTarget& page, float dt, bool active = true);
	void begin_keyboard_capture();
	void capture_input_frame(DebugPageTarget& page);
	void end_keyboard_capture();
	bool keyboard_jump_collision(labrador::Key key) const;

	// The console and the debug menu report their own lifetimes, so only one
	// of each is ever open.
	void set_console_open(bool open);
	bool console_open() const;
	bool blocking_console_open() const { return this->console_open_; }
	void set_debug_menu(DebugMenuPage* page);
	bool debug_menu_open() const { return this->debug_menu_ != nullptr; }

	// The debug menu's own input and context, made on first use: it is pushed
	// over any state, so it borrows none of theirs.
	MenuContext* menu_context();

	// Short "x0.25 PAUSED feedback-off GOD" text, empty when all default.
	std::string status_strip() const;

	// "paint  A 23.4%  B 11.0%  free 65.6%" and a per-seat line, or empty
	// when overlay.paint is off or no match is running.
	std::vector<std::string> paint_lines() const;

	// What `players`, `stage` and `reload` read and queue.
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

	// Phase 3 ---------------------------------------------------------------

	// GameLevel's hooks (DebugDirector::match_suspended, drives_slot).
	void match_suspended(DebugMatchTarget& match);
	bool drives_slot(int slot) const;
	OpeningMode opening_mode(OpeningMode chosen) const;

	// Live tuning reads the baseline from the staged copy and tune.save
	// writes the source tree. Empty: the build's source tree
	// (game/source_paths.h) and the executable's directory. The acceptance
	// case points both at temporary copies.
	void set_content_directories(std::filesystem::path source, std::filesystem::path staged);
	std::filesystem::path source_content_directory() const;
	std::filesystem::path staged_content_directory() const;

	// "tuning" or "presentation"; null when the file could not be read.
	TuneDocument* tune_document(std::string_view file);
	// Every tunable path, both files, prefixed, in file order.
	std::vector<std::string> tune_paths();
	// The token a prefixed path holds now, or empty.
	std::string tune_token(const std::string& path);
	// Changes whenever tune_paths() could: a document loaded or dropped (the
	// first element), or one committed or rebased (its revision, -1 when the
	// file could not be read).
	std::array<int, 3> tune_revision();

	// Level updates since the running match was built.
	int level_updates() const { return this->level_updates_; }
	bool recording() const { return this->recorder_ != nullptr; }
	bool playing() const { return this->player_ != nullptr; }
	bool replay_finished() const;
	const std::optional<unsigned int>& seed() const { return this->seed_; }

	// The bot behaviour driving a seat (0-based) of the running or next
	// match, or empty: an explicit `bot <seat>` first, then `bot all` for an
	// idle dummy. A shipping bot is never an idle dummy.
	std::string bot_for_seat(int seat, int slot, bool shipping_bot = false) const;

	// The running match's fingerprint, or nullopt with no match.
	std::optional<MatchFingerprint> fingerprint() const;

	// Phase 4 ---------------------------------------------------------------

	// Where autoexec.cfg, binds.cfg, history.txt and the player's scripts
	// live: the override, else %LOCALAPPDATA%\ColourWars\debug once attach()
	// had an application, else nothing - then all three are memory-only and
	// no script is read from a profile (the acceptance case without an
	// override never touches the developer's own files).
	void set_user_directory(std::filesystem::path directory);
	std::optional<std::filesystem::path> user_directory() const;
	// Where exec looks, in order: the override, else the source tree's
	// scripts/debug (game/source_paths.h), then the user directory.
	void set_script_directories(std::vector<std::filesystem::path> directories);
	std::vector<std::filesystem::path> script_directories() const;

	DebugScriptRunner& scripts() { return this->scripts_; }
	DebugWaitContext wait_context() const;
	long long total_updates() const { return this->total_updates_; }
	const DebugBinds& binds() const { return this->binds_; }
	const std::string& screenshot_dir() const { return this->screenshot_dir_; }

	// The live console: open over a running match while the match keeps
	// going, the keyboard seat blanked. The setter is the acceptance case's
	// seam; it opens no view.
	bool live_console_open() const { return this->live_open_; }
	void set_live_console_open(bool open);

	DebugPerf& perf() { return this->perf_; }

	// The debug menu runs staged lines through this: a staged rebuild says it
	// waits for the menu to close instead of "rebuilding the match".
	void set_staging(bool staging) { this->staging_ = staging; }
	// ": rebuilding the match", or ": queued; rebuilds when the debug menu
	// closes" while it is open or a staged line runs.
	std::string rebuild_note() const;

	// What each seat's HUD says instead of its device: "Replay" while a
	// replay drives the match, "Bot: <behaviour>" for a bot's seat, else "".
	std::vector<std::wstring> seat_labels(const DebugMatchTarget& match) const;

	// DebugDirector::begin_match_draw and draw_status.
	void begin_match_draw();
	void draw_status(labrador::Renderer& renderer);

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

	bool keyboard_capture_ = false;
	bool background_scripts_ = false;
	bool background_audio_muted_ = false;
	bool match_hook_active_ = true;
	DebugBackgroundInput background_inputs_;
	DebugBackgroundWindow background_window_;
	std::function<DebugBackgroundState()> background_probe_;
	void background_changed();

	// `intro`: empty for the game's own choice, else full, quick or off.
	std::string intro_;

	// Content validated by `reload`, assigned before the next level is built.
	std::unique_ptr<GameTuning> pending_tuning_;
	std::unique_ptr<Presentation> pending_presentation_;

	// Overlays and cheats live here, not in the level, so every build
	// re-applies them (after_level_build).
	bool overlay_player_ = false;
	bool overlay_collision_ = false;
	bool overlay_camera_ = false;
	bool overlay_paint_ = false;
	bool overlay_stations_ = false;
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
	bool open_console_if_requested(bool match_hook);
	bool poll_debug_menu_toggle(float dt);
	DebugChord::Pads debug_pads() const;
	void cancel_capture_work();
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

	void log_match(const MatchPlan& plan, const std::string& how);

	void apply_overlays(bool player_text_changed);
	// The level's share of the overlay variables (player text and paint are
	// pushed or drawn separately).
	LevelOverlays level_overlays() const;
	void apply_cheats();
	DebugStatus apply_goto(DebugMatchTarget& match, const std::string& station, int seat);

	// Takes a pending screenshot when nothing queued would change the picture
	// before the next frame.
	void capture_screenshot_if_ready();

	DebugStatus reload_content();
	std::vector<std::string> status_lines() const;

	// Phase 3 ---------------------------------------------------------------

	void register_tune_commands();
	void register_bot_commands();
	void register_replay_commands();

	// Seed: `seed n` makes rand() (menus, palettes) and the planner's random
	// choices repeat; without one the planner draws from random_device.
	std::optional<unsigned int> seed_;
	std::mt19937 planner_random_;
	MatchPlanner::Random planner_random();

	// Bots: explicit seats (0-based) and the behaviour for every idle dummy.
	std::map<int, std::string> seat_bots_;
	std::string dummy_bot_;
	std::unique_ptr<BotDriver> bot_driver_;
	void rebuild_bots(const DebugMatchTarget& match);
	std::string bot_summary() const;

	// Record and play. Recording starts at a fresh build; so does playing.
	int level_updates_ = 0;
	std::uint32_t last_dt_bits_ = 0;
	struct RecordRequest
	{
		std::string name;
		int limit = 0;
	};
	std::optional<RecordRequest> record_armed_;
	std::unique_ptr<ReplayRecorder> recorder_;
	std::optional<MatchFingerprint> last_print_;
	std::unique_ptr<ReplayPlayer> play_armed_;
	std::unique_ptr<ReplayPlayer> player_;
	bool injecting_suspend_ = false;
	std::optional<std::pair<bool, bool>> cheats_before_play_;
	SeatInputFilter* seat_filter();
	void before_level_update(DebugMatchTarget& match, float dt);
	void after_level_update(DebugMatchTarget& match);
	void stop_recording(const std::string& reason);
	void stop_playing(const std::string& reason);
	void restore_cheats_after_play();
	// Drops an armed replay whose build will not come: the exact request that
	// carries it was dropped, failed to plan, or the match was left.
	void disarm_play(const std::string& reason);
	// The clock was paused by a replay holding its last frame (not by the
	// user): the next replay to start resumes it.
	bool replay_hold_ = false;
	ReplayFile replay_header(const DebugMatchTarget& match);
	// FNV-1a of a content file's text as live tuning holds it (0 unreadable).
	std::uint64_t content_hash(std::string_view file);

	// Refuses commands that change the simulation outside the input stream
	// while a recording or a replay runs.
	std::optional<DebugStatus> refuse_while_replaying(const std::string& what) const;

	// Live tuning.
	std::filesystem::path source_override_;
	std::filesystem::path staged_override_;
	std::array<std::unique_ptr<TuneDocument>, 2> tune_docs_;
	std::array<std::unique_ptr<TuneTarget>, 2> tune_targets_;
	bool tune_load_tried_ = false;
	// Bumped whenever tune_docs_ are replaced; see tune_revision().
	int tune_generation_ = 0;
	// The texts `reload` validated, which become the tuning baseline when
	// they are applied.
	std::string pending_tuning_text_;
	std::string pending_presentation_text_;
	void load_tune_documents();
	int tune_file_index(std::string_view file) const;
	DebugStatus tune_value(const std::string& path, const std::string& word, bool restart);
	DebugStatus tune_validate_and_apply(int file, const std::string& candidate,
		const std::string& edited_path);
	DebugStatus tune_reset(const std::string& what);
	DebugStatus tune_save(const std::string& which, bool force);
	std::vector<std::string> tune_diff_lines(const std::string& which);

	// Phase 4 ---------------------------------------------------------------

	void register_script_commands();
	void register_bind_commands();

	// Scripts. Ticked only from the two hooks (and once, synchronously, by
	// the launch arguments); a script never runs inside an update.
	DebugScriptRunner scripts_;
	long long total_updates_ = 0;
	// Every hook frame, and the hook frames on which a script could tick (what
	// `wait frames` counts: not under the debug menu, nor on a frame the menu
	// toggle or the console key takes).
	long long hook_frames_ = 0;
	long long script_frames_ = 0;
	// While a hook ticks: a pending screenshot may be taken in place, so two
	// shots in a row show the same simulation frame.
	bool inline_capture_ = false;
	void tick_scripts(bool from_hook, bool background_match = false);
	DebugStatus exec_script(const std::string& name, const std::vector<std::string>& args);
	DebugStatus wait_command(const DebugArgs& args);

	// Persistence (binds.cfg, history.txt), loaded once.
	std::filesystem::path user_override_;
	std::vector<std::filesystem::path> script_override_;
	bool persistence_loaded_ = false;
	void load_persistent();
	void load_binds(const std::filesystem::path& directory);
	void save_binds();
	void save_history();

	// Binds: the F2-F7 and F9 defaults until `bind` changes them.
	DebugBinds binds_;
	bool binds_loading_ = false;
	// A history.txt write failed; said once a session.
	bool history_write_failed_ = false;

	// Screenshots: a folder under screens/ for a review set, and whether the
	// status strip shows in a capture.
	std::string screenshot_dir_;
	bool screenshot_strip_ = true;
	bool capturing_ = false;

	// The live console.
	bool live_console_ = false;
	std::unique_ptr<DebugConsoleView> live_view_;
	bool live_open_ = false;
	// The frame the live console closed on (the Escape release, which is the
	// keyboard player's pause key) is blanked too; filter_match_inputs clears it.
	mutable bool blank_keyboard_once_ = false;
	void open_live_console();
	void close_live_console(const std::string& reason);

	// The perf overlay.
	DebugPerf perf_;
	bool overlay_perf_ = false;
	// The last level build's wall time, from the end of before_level_build
	// to the top of after_level_build, and what was built.
	struct LevelBuildTime
	{
		double ms = 0.0;
		std::string stage;
		int players = 0;
	};
	double build_began_ms_ = -1.0;
	std::optional<LevelBuildTime> last_build_;
	std::optional<std::filesystem::path> perf_capture_path_;
	int perf_width_ = 0;
	int perf_height_ = 0;
	DebugStatus start_perf_capture(const DebugArgs& args);
	void finish_perf_capture();
	void break_perf_interval();

	bool staging_ = false;
	void push_seat_labels(DebugMatchTarget& match);
};

