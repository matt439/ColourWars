#pragma once

#include "engine/core/state.h"
#include "game/session.h"
#include "game/match_setup.h"
#include "game/game_data.h"
#include "game/objects/level_builder.h"
#include "game/states/pause_menu.h"
#include "game/states/results_menu.h"
#include "game/states/main_menu.h"
#include "game/states/end_menu.h"
#include "game/objects/player_input.h"
#include "game/input_slots.h"
#include "game/debug/debug_targets.h"
#include "game/states/resume_countdown.h"
#include "game/states/resume_audio.h"
#include "game/states/results_audio.h"
#include "game/states/match_start.h"
#include "game/display_awake.h"
#include "game/rumble_sink.h"
#include "game/round_tape.h"

// Stack shapes during a session (docs/architecture.md lists every overlay):
//
//     [GameMenu, MainMenu*Page]
//     [GameMenu, MainMenuOptions, ControlsSettingsMenu, <a controls page>]
//     [GameMenu, GameLevel]
//     [GameMenu, GameLevel, PauseMenu*Page | ResultsMenuInitial | EndMenuInitial]
//     [GameMenu, GameLevel, MainMenuWeaponSelect | MainMenuStageSelect]   (between rounds)
//     [GameMenu, GameLevel, PauseMenuInitial, MainMenuOptions, ControlsSettingsMenu]
//
// GameMenu is alone only during transitions. Its context and settings survive
// matches; menu pages are rebuilt when returning from a match.

class UserStageFolder;

// Owns shared menu state and routes between setup pages and matches.
class GameMenu final : public labrador::State
{
public:
	// `user_stages`, when given, is rescanned as Stage Select opens from the
	// main menu (ROAD#16); probes leave it out and keep their stage list.
	explicit GameMenu(GameData* game_data, UserStageFolder* user_stages = nullptr);
	void update(float dt) override;
	void draw(labrador::Renderer& renderer) const override;
	void init() override;
	void on_deactivated() override;

private:
	GameData* game_data_ = nullptr;
	UserStageFolder* user_stages_ = nullptr;
	std::unique_ptr<MenuInput> menu_input_ = nullptr;
	std::unique_ptr<MatchSetup> match_setup_ = nullptr;
	std::unique_ptr<MenuContext> menu_context_ = nullptr;
	std::unique_ptr<ScreenMusic> music_;
	void update_music(float dt);

	void open_menu(MainMenuScreen screen);
	std::unique_ptr<MainMenuPage> build_page(MainMenuScreen screen) const;

	void start_match(MatchStart start);
	void on_match_ended(const EndMenuAction& action);
	GameData* data() const;
};

// A match or series, with a fresh Level for each round. The developer tools
// reach it only through DebugMatchTarget, from hooks at the top of update().
class GameLevel final : public labrador::State, private DebugMatchTarget
{
public:
	GameLevel(GameData* game_data, const MatchSetup& settings,
		MatchStart start = MatchStart::developer);

	// Restores the screen layout that was active before the match.
	~GameLevel() override;

	void update(float dt) override;
	void draw(labrador::Renderer& renderer) const override;
	void init() override;
	// Read-only match/transition status; observing it never advances either clock.
	const Level& level() const;
	float resume_remaining() const;

	// Stack coverage stops simulation updates; these callbacks suspend and
	// resume the audio that continues independently.
	void on_suspend() override;
	void on_resume() override;

	// Focus and stack coverage are independent reasons to suspend audio.
	// Regaining either resumes only when the other also permits playback.
	void on_activated() override;
	void on_deactivated() override;

private:
	// Levels this screen has built: the first has the whole intro.
	int builds_ = 0;
	GameData* game_data_ = nullptr;
	MatchSetup settings_ = MatchSetup();
	MatchStart start_ = MatchStart::developer;
	Session session_;
	// The sink outlives every policy, including reset/unwind during a rebuild.
	std::unique_ptr<RumbleSink> rumble_sink_;
	std::unique_ptr<Rumble> rumble_;
	std::unique_ptr<Level> level_ = nullptr;
	std::unique_ptr<LevelBuilder> level_builder_ = nullptr;
	std::unique_ptr<PlayerInput> player_input_ = nullptr;
	std::unique_ptr<MenuInput> menu_input_ = nullptr;

	// The end-of-round instant replay (ROAD#19, game_level_replay.cpp). Each
	// timed round is taped as it is played; Watch Replay on the End menu
	// builds the level again from the tape, runs it silently to its last
	// seconds, plays those and returns to the End menu.
	RoundTape tape_;
	RoundTapeRecorder tape_recorder_{ &this->tape_ };
	enum class ReplayPhase { none, preparing, playing };
	ReplayPhase replay_phase_ = ReplayPhase::none;
	std::unique_ptr<RoundTapePlayer> replay_player_;
	int replay_first_ = 0;
	void start_replay();
	// True while a replay owns this update.
	bool update_replay(float dt);
	void step_replay();
	// Back to the End menu over the replay's last frame.
	void finish_replay();
	// Drops any replay and gives the player's volumes back to the sound groups.
	void end_replay();
	// Every suspend of the live round, so the tape has it.
	void suspend_level();

	bool covered_ = false;
	bool pause_menu_open_ = false;
	bool focus_interrupted_ = false;
	bool background_continuing_ = false;
	ResumeCountdown resume_countdown_;
	std::unique_ptr<ResumeAudio> resume_audio_;
	std::unique_ptr<ResultsAudio> results_audio_;
	void begin_resume_countdown();
	// Per device slot: a Start or Escape press that began over the intro. Its
	// release never opens pause, even when it comes after GO.
	std::array<bool, input_slots::count> pause_over_intro_{};

	// Shared context for menus above the match; settings_ outlives those pages.
	std::unique_ptr<MenuContext> menu_context_ = nullptr;

	// Restored by the destructor.
	labrador::ScreenLayout previous_layout_ = labrador::ScreenLayout::one_player;

	// Sets the layout before building a level, including every restart. The
	// one build path: between destroying the old level and building the new
	// one it lets the debug director apply reloaded content and adopt a queued
	// plan, and once the level exists, re-apply its overlays and cheats
	// (no-ops in Release).
	void build_and_enter_level();

	// Gameplay voices play only when nothing covers the match, the window is
	// active and the debug clock is not paused. Arena music may remain ducked.
	bool audio_may_play() const;
	void update_music(float dt);
	void stop_rumble() const noexcept;
	void sync_rumble();
	void drain_rumble(float dt);

	// DebugMatchTarget adapters. debug_update_level also carries ordinary Release
	// updates through DebugDirector's forwarding hook and the shipping recorder.
	const MatchSetup& debug_settings() const override;
	void debug_rebuild(const MatchSetup& plan, bool reset_session) override;
	void debug_adopt(const MatchSetup& plan, bool reset_session) override;
	void debug_leave() override;
	void debug_update_level(const std::vector<PlayerInputData>& inputs,
		float dt, SeatInputFilter* seats) override;
	bool debug_level_finished() const override;
	const Level* debug_level() const override;
	void debug_sync_audio() override;
	void debug_background_changed() override;
	std::vector<std::string> debug_status() const override;
	void debug_set_overlays(const LevelOverlays& overlays) override;
	void debug_set_player_text(bool showing) override;
	void debug_set_cheats(bool damage_immune, bool infinite_ammo) override;
	std::vector<LevelStation> debug_stations() const override;
	bool debug_place(int seat, const mattmath::Vector2F& top_left) override;
	bool debug_respawn(int seat) override;
	bool debug_kill(int seat) override;
	MatchPaint debug_paint() const override;
	bool debug_countdown_over() const override;
	bool debug_perf_ready() const override;
	void debug_set_seat_labels(const std::vector<std::wstring>& labels) override;
	std::vector<mattmath::RectangleF> debug_screen_obstacles() const override;

	void open_pause_menu(int player_num);
	void on_pause_menu_closed(const PauseMenuAction& action);
	void open_results_menu();
	void open_end_menu();
	void on_end_menu_closed(const EndMenuAction& action);

	void leave_match(EndMenuAction action) const;

	// Returns the input slot of a participant requesting pause, or -1.
	// Unclaimed devices cannot open the pause menu.
	int check_for_pause_input(
		const std::vector<PlayerInputData>& player_inputs) const;
	GameData* data() const;

	// Last member: acquire after earlier members and clear on member unwind.
	// Pause/focus/round changes keep this same GameLevel owner alive.
	DisplayAwake display_awake_;
};
