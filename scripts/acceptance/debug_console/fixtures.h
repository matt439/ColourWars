#pragma once

// What the debug_console acceptance pieces share: the fakes the director is
// driven against, the test director and stages, the scratch folders, the log
// and file helpers, and the checks main calls. Release uses require, FakeMatch,
// FakePage and the loaded-stage semantic checks.
#include "game/debug/dev_tools.h"
#include "game/debug/debug_director.h"
#include "game/input_slots.h"
#include "game/presentation.h"
#include "game/tuning.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_log.h"
#include "game/debug/debug_menu_model.h"
#include "game/debug/match_plan.h"
#include "game/game_data.h"
#include <process.h>
#endif

namespace debug_console_acceptance
{
	void require(bool condition, const std::string& message);
	void check_loaded_stage_semantics();

	// A match as the director sees it: counts updates and rebuilds, records
	// each update's jump/debug/intro edges, and can finish after a number of updates.
	// Like GameLevel, its rebuild goes through before_level_build. Compiled in
	// Release too, where the director must ask it nothing.
	class FakeMatch final : public DebugMatchTarget
	{
	public:
		explicit FakeMatch(DebugDirector& director) : director_(director) {}

		MenuLevelSettings settings;
		int updates = 0;
		int finish_after = -1;
		std::vector<bool> jumps;
		int debug_edges = 0;
		int intro_skip_edges = 0;
		int rebuilds = 0;
		bool last_reset_session = false;
		int leaves = 0;
		int audio_syncs = 0;
		int background_changes = 0;

		// Phase 2: what the director pushed into the level, and what it asked of it.
		LevelOverlays overlays;
		bool player_text = false;
		int overlay_sets = 0;
		int player_text_sets = 0;
		int cheat_sets = 0;
		bool damage_immune = false;
		bool infinite_ammo = false;
		int adopts = 0;
		std::vector<std::pair<int, mattmath::Vector2F>> placed;
		std::vector<int> respawned;
		std::vector<int> killed;

		// Phase 3: the seat filter each update was given.
		SeatInputFilter* last_filter = nullptr;
		int filtered_updates = 0;

		const MenuLevelSettings& debug_settings() const override { return this->settings; }
		void debug_rebuild(const MenuLevelSettings& plan, bool reset_session) override
		{
			this->settings = plan;
			++this->rebuilds;
			this->last_reset_session = reset_session;
			this->director_.before_level_build(*this);
			this->director_.after_level_build(*this);
		}
		void debug_adopt(const MenuLevelSettings& plan, bool reset_session) override
		{
			this->settings = plan;
			++this->adopts;
			this->last_reset_session = reset_session;
		}
		void debug_leave() override { ++this->leaves; }
		void debug_set_overlays(const LevelOverlays& value) override
		{
			this->overlays = value;
			++this->overlay_sets;
		}
		void debug_set_player_text(bool showing) override
		{
			this->player_text = showing;
			++this->player_text_sets;
		}
		void debug_set_cheats(bool immune, bool infinite) override
		{
			++this->cheat_sets;
			this->damage_immune = immune;
			this->infinite_ammo = infinite;
		}
		std::vector<LevelStation> debug_stations() const override
		{
			return { { "high_step", { 4404.0f, 535.0f } }, { "spawn_a", { 100.0f, 875.0f } } };
		}
		bool debug_place(int seat, const mattmath::Vector2F& top_left) override
		{
			if (seat < 0 || seat >= this->settings.player_count()) { return false; }
			this->placed.push_back({ seat, top_left });
			return true;
		}
		bool debug_respawn(int seat) override
		{
			if (seat < 0 || seat >= this->settings.player_count()) { return false; }
			this->respawned.push_back(seat);
			return true;
		}
		bool debug_kill(int seat) override
		{
			if (seat < 0 || seat >= this->settings.player_count()) { return false; }
			this->killed.push_back(seat);
			return true;
		}
		MatchPaint debug_paint() const override
		{
			MatchPaint paint;
			paint.team_a = 30.0f;
			paint.team_b = 10.0f;
			paint.total = 100.0f;
			paint.seats = { 30.0f, 0.0f, 0.0f, 0.0f };
			paint.seats.resize(static_cast<std::size_t>(this->settings.player_count()));
			return paint;
		}
		void debug_update_level(const std::vector<PlayerInputData>& inputs, float,
			SeatInputFilter* seats) override
		{
			++this->updates;
			this->jumps.push_back(
				inputs[static_cast<std::size_t>(input_slots::keyboard_and_mouse)].jump_pressed);
			const auto& keyboard = inputs[static_cast<std::size_t>(input_slots::keyboard_and_mouse)];
			this->debug_edges += keyboard.toggle_debug ? 1 : 0;
			this->intro_skip_edges += keyboard.skip_intro ? 1 : 0;
			this->last_filter = seats;
			this->filtered_updates += seats != nullptr ? 1 : 0;
		}
		bool debug_level_finished() const override
		{
			return this->finish_after >= 0 && this->updates >= this->finish_after;
		}
		const Level* debug_level() const override { return nullptr; }
		void debug_sync_audio() override { ++this->audio_syncs; }
		void debug_background_changed() override { ++this->background_changes; }
		std::vector<std::string> debug_status() const override { return {}; }

		// Phase 4: the countdown, the HUD labels pushed, the opening cards.
		bool countdown_over = true;
		std::vector<std::wstring> seat_labels;
		int seat_label_sets = 0;
		std::vector<mattmath::RectangleF> obstacles;
		bool debug_countdown_over() const override { return this->countdown_over; }
		void debug_set_seat_labels(const std::vector<std::wstring>& labels) override
		{
			this->seat_labels = labels;
			++this->seat_label_sets;
		}
		std::vector<mattmath::RectangleF> debug_screen_obstacles() const override
		{
			return this->obstacles;
		}

		int jumps_delivered() const
		{
			return static_cast<int>(std::count(this->jumps.begin(), this->jumps.end(), true));
		}
		void reset_counts()
		{
			this->updates = 0;
			this->jumps.clear();
			this->debug_edges = 0;
			this->intro_skip_edges = 0;
			this->finish_after = -1;
		}

	private:
		DebugDirector& director_;
	};

	// A page as the director sees it: records what it was asked to do.
	class FakePage final : public DebugPageTarget
	{
	public:
		bool start_result = false;
		bool dismiss_result = true;
		int starts = 0;
		int dismissals = 0;
		int resets = 0;
		std::vector<int> reset_slots;
		bool last_leaving = false;
		MenuLevelSettings started;

		bool debug_start_match(const MenuLevelSettings& plan) override
		{
			++this->starts;
			this->started = plan;
			return this->start_result;
		}
		bool debug_dismiss(bool leaving_match) override
		{
			++this->dismissals;
			this->last_leaving = leaving_match;
			return this->dismiss_result;
		}
		void debug_reset_input(int pad_slot) override
		{
			++this->resets;
			this->reset_slots.push_back(pad_slot);
		}
	};

#if COLOURWARS_DEV_TOOLS
	bool log_contains(const DebugLog& log, const std::string& text, DebugSeverity severity);
	const StageList& test_stages();
	MenuLevelSettings planned(int players);

	// A director over the two test stages, with no application: what the
	// hooks do without a window.
	struct TestDirector
	{
		GameData data;
		DebugDirector director;
		DebugDirectorImpl& impl;

		TestDirector(GameTuning& tuning, Presentation& presentation) :
			director(&tuning, &presentation), impl(*director.impl())
		{
			this->data.set_stages(&test_stages());
			this->director.attach(nullptr, &this->data);
			this->impl.log().set_mirror_to_debugger(false);
		}
	};

	std::size_t row_index(const std::vector<DebugMenuRow>& rows, const std::string& label);
	std::vector<std::string> labels(const std::vector<DebugMenuRow>& rows);
	std::string read_text(const std::filesystem::path& path);
	void write_text(const std::filesystem::path& path, const std::string& text);
	std::string to_lf(std::string text);
	std::size_t count_of(const std::string& text, const std::string& what);

	// A scratch content tree: source/ and staged/ copies of both files, the
	// tuning file CRLF and the presentation file LF, so tune.save is checked
	// on both line endings. Removed on destruction; the real files are only
	// ever read.
	struct ScratchContent
	{
		std::filesystem::path root;
		std::filesystem::path source;
		std::filesystem::path staged;

		explicit ScratchContent(const std::filesystem::path& game_directory, const wchar_t* tag)
		{
			this->root = std::filesystem::temp_directory_path() /
				(std::wstring(L"colourwars_phase3_") + tag + L"_" + std::to_wstring(_getpid()));
			std::error_code ignored;
			std::filesystem::remove_all(this->root, ignored);
			this->source = this->root / L"source";
			this->staged = this->root / L"staged";
			std::filesystem::create_directories(this->source);
			std::filesystem::create_directories(this->staged);
			std::string tuning = to_lf(read_text(game_directory / L"tuning.json"));
			std::string crlf;
			for (const char c : tuning)
			{
				crlf += c == '\n' ? std::string("\r\n") : std::string(1, c);
			}
			const std::string presentation = to_lf(read_text(game_directory / L"presentation.json"));
			for (const std::filesystem::path& directory : { this->source, this->staged })
			{
				write_text(directory / L"tuning.json", crlf);
				write_text(directory / L"presentation.json", presentation);
			}
		}

		~ScratchContent()
		{
			std::error_code ignored;
			std::filesystem::remove_all(this->root, ignored);
		}

		ScratchContent(const ScratchContent&) = delete;
		ScratchContent& operator=(const ScratchContent&) = delete;
	};

	// A scratch folder that removes itself.
	struct ScratchFolder
	{
		std::filesystem::path root;
		explicit ScratchFolder(const std::wstring& name) :
			root(std::filesystem::temp_directory_path() /
				(L"colourwars_" + name + L"_" + std::to_wstring(_getpid())))
		{
			std::error_code ignored;
			std::filesystem::remove_all(this->root, ignored);
			std::filesystem::create_directories(this->root);
		}
		~ScratchFolder()
		{
			std::error_code ignored;
			std::filesystem::remove_all(this->root, ignored);
		}
		ScratchFolder(const ScratchFolder&) = delete;
		ScratchFolder& operator=(const ScratchFolder&) = delete;
	};

	std::size_t log_index(const DebugLog& log, const std::string& text);
	bool any_line(const DebugLog& log, const std::string& text);

	// The fifty-eight audited colour tune paths and their values, in the same order;
	// shared by tune_checks.cpp, tune_document_checks.cpp and tune_menu_checks.cpp.
	inline constexpr std::size_t identity_ink_colour_index = 13;
	inline constexpr std::size_t identity_background_colour_index = 14;
	inline constexpr std::size_t combat_highlight_colour_index = 15;
	inline constexpr std::size_t combat_keyline_colour_index = 16;
	inline constexpr std::size_t blocked_highlight_colour_index = 17;
	inline constexpr std::size_t blocked_keyline_colour_index = 18;
	inline constexpr std::size_t idle_drip_keyline_colour_index = 19;
	inline constexpr std::size_t respawn_pen_colour_index = 20;
	inline constexpr std::size_t cursor_ink_colour_index = 21;
	inline constexpr std::size_t cursor_fill_colour_index = 22;
	inline constexpr std::size_t silhouette_ink_colour_index = 23;
	inline constexpr std::size_t silhouette_rim_colour_index = 24;
	inline constexpr std::size_t silhouette_shade_colour_index = 25;
	inline constexpr std::size_t reveal_ink_colour_index = 26;
	inline constexpr std::size_t reveal_paper_colour_index = 27;
	inline constexpr std::size_t reveal_shade_colour_index = 28;
	inline constexpr std::size_t cursor_shade_colour_index = 29;
	inline constexpr std::size_t cursor_shadow_colour_index = 30;
	inline constexpr std::size_t desktop_colour_index = 31;
	inline constexpr std::size_t power_light_colour_index = 32;
	inline constexpr std::size_t program_colour_index = 33;
	inline constexpr std::size_t acting_face_colour_index = 34;
	inline constexpr std::size_t acting_sweat_colour_index = 35;
	inline constexpr std::size_t glass_colour_index = 36;
	inline constexpr std::size_t silhouette_body_colour_index = 37;
	inline constexpr std::size_t scribble_colour_index = 38;
	inline constexpr std::size_t hud_health_colour_index = 39;
	inline constexpr std::size_t hud_track_colour_index = 40;
	inline constexpr std::size_t crack_colour_index = 41;
	inline constexpr std::size_t limb_arm_colour_index = 42;
	inline constexpr std::size_t limb_hand_colour_index = 43;
	inline constexpr std::size_t quiet_face_colour_index = 44;
	inline constexpr std::size_t hud_timer_colour_index = 45;
	inline constexpr std::size_t hud_timer_shadow_colour_index = 46;
	inline constexpr std::size_t closing_clock_text_colour_index = 47;
	inline constexpr std::size_t hud_label_colour_index = 48;
	inline constexpr std::size_t hud_identity_shadow_colour_index = 49;
	inline constexpr std::size_t stage_preview_background_colour_index = 50;
	inline constexpr std::size_t closing_clock_ink_colour_index = 51;
    inline constexpr std::size_t closing_clock_paper_colour_index = 52;
    inline constexpr std::size_t closing_clock_accent_colour_index = 53;
    // Independent reader contract: these four exact closing fields are opaque.
    inline constexpr bool opaque_closing_colour_index(std::size_t index)
    {
        return index == closing_clock_text_colour_index || index == closing_clock_ink_colour_index ||
            index == closing_clock_paper_colour_index || index == closing_clock_accent_colour_index;
    }
    inline constexpr std::size_t closing_clock_shade_colour_index = 54;
    inline constexpr std::size_t hud_paper_ink_colour_index = 55;
    inline constexpr std::size_t hud_paper_paper_colour_index = 56;
    inline constexpr std::size_t hud_paper_shade_colour_index = 57;
    // Shared HUD paper has its own coupled opaque/distinct reader contract.
    inline constexpr bool opaque_hud_paper_colour_index(std::size_t index)
    {
        return index == hud_paper_ink_colour_index || index == hud_paper_paper_colour_index;
    }
    inline constexpr bool hud_paper_colour_index(std::size_t index)
    {
        return opaque_hud_paper_colour_index(index) || index == hud_paper_shade_colour_index;
    }
    // Positive alpha is independent of family routing and opaque/zero-allowed fields.
    inline constexpr bool positive_alpha_colour_index(std::size_t index)
    {
        return index == closing_clock_shade_colour_index || index == hud_paper_shade_colour_index;
    }
    inline const std::array<const char*, 58> colour_paths{
		"presentation.movement_feedback.colour", "presentation.movement_feedback.highlight",
		"presentation.movement_feedback.keyline", "presentation.debug_overlay.colour",
		"presentation.debug_overlay.shadow_colour", "presentation.debug_overlay.station_label.colour",
		"presentation.main_menu.backgrounds.home", "presentation.main_menu.team_select.selected",
		"presentation.main_menu.team_select.unselected", "presentation.main_menu.weapon_select.selected",
		"presentation.main_menu.weapon_select.unselected", "presentation.weapon_feedback.highlight",
		"presentation.weapon_feedback.keyline", "presentation.player_identity.ink_colour",
		"presentation.player_identity.background_colour", "presentation.combat_feedback.highlight",
		"presentation.combat_feedback.keyline", "presentation.blocked_shot.highlight",
		"presentation.blocked_shot.keyline", "presentation.idle_drip.keyline",
		"presentation.respawn_redraw.pen_colour", "presentation.opening.cursor_art.ink",
		"presentation.opening.cursor_art.fill", "presentation.opening.silhouette_art.ink",
		"presentation.opening.silhouette_art.rim", "presentation.opening.silhouette_art.shade",
		"presentation.opening.reveal_art.ink", "presentation.opening.reveal_art.paper",
		"presentation.opening.reveal_art.shade", "presentation.opening.cursor_art.shade",
		"presentation.opening.cursor_art.shadow", "presentation.opening.desktop_colour",
		"presentation.opening.monitor_art.power_light", "presentation.opening.program_colour",
		"presentation.opening.acting.face", "presentation.opening.acting.sweat",
		"presentation.opening.glass_colour", "presentation.opening.silhouette", "presentation.opening.scribble", "presentation.hud.health_colour", "presentation.hud.meter_track_colour", "presentation.opening.crack", "presentation.figure.arm_colour", "presentation.figure.hand_colour", "presentation.figure.quiet.face_colour", "presentation.hud.timer.colour", "presentation.hud.timer.shadow_colour", "presentation.hud.closing_clock.text", "presentation.hud.player_identity.colour", "presentation.hud.player_identity.shadow_colour", "presentation.hud.player_identity.background_colour", "presentation.hud.closing_clock.ink", "presentation.hud.closing_clock.paper", "presentation.hud.closing_clock.accent", "presentation.hud.closing_clock.shade", "presentation.hud.paper.ink", "presentation.hud.paper.paper", "presentation.hud.paper.shade" };
	std::vector<labrador::Colour> colour_values(const Presentation& presentation);

	// console_checks.cpp
	void check_tokenizer();
	void check_launch_arguments();
	void check_registry();
	void check_clock();
	void check_planner();
	void check_text_field();

	// director_checks.cpp
	void check_match_hooks(GameTuning& tuning, Presentation& presentation);
	void check_director(GameTuning& tuning, Presentation& presentation);
	void check_unwinding(GameTuning& tuning, Presentation& presentation);
	void check_overlays_and_cheats(GameTuning& tuning, Presentation& presentation);
	void check_goto(GameTuning& tuning, Presentation& presentation);
	void check_screenshot_file(GameTuning& tuning, Presentation& presentation);
	void check_stations(const std::filesystem::path& game_directory, const GameTuning& tuning);

	// background_checks.cpp
	void check_background_policy(const std::filesystem::path& content, GameTuning& tuning,
		Presentation& presentation);

	// menu_checks.cpp
	void check_chord();
	void check_input_filter(GameTuning& tuning, Presentation& presentation);
	void check_registry_menu_traits(GameTuning& tuning, Presentation& presentation);
	void check_menu_model(GameTuning& tuning, Presentation& presentation);

	// tune_document_checks.cpp
	void check_json_spans(const std::filesystem::path& game_directory);
	void check_tune_scope(const std::filesystem::path& game_directory);
	int check_tune_consumed(const std::filesystem::path& game_directory);

	// tune_checks.cpp
	void check_tune_commands(const std::filesystem::path& game_directory);

	// tune_menu_checks.cpp
	void check_tune_completion_and_menu(const std::filesystem::path& game_directory);

	// bot_checks.cpp
	void check_bots(GameTuning& tuning, Presentation& presentation);
	void check_seeded_planner(GameTuning& tuning, Presentation& presentation);

	// replay_checks.cpp
	void check_replay_codec();
	std::string check_replay_cpu_match(const std::filesystem::path& game_directory);

	// level_checks.cpp
	void check_opening(const std::filesystem::path& game_directory);
	void check_station_labels(const std::filesystem::path& game_directory);

	// script_checks.cpp
	void check_script_parse();
	void check_script_runner(GameTuning& tuning, Presentation& presentation);
	void check_shipped_scripts(GameTuning& tuning, Presentation& presentation,
		const std::filesystem::path& game_directory);
	void check_wait_failures();

	// session_checks.cpp
	void check_binds(GameTuning& tuning, Presentation& presentation);
	void check_history(GameTuning& tuning, Presentation& presentation);
	void check_live_console_input(GameTuning& tuning, Presentation& presentation);
	void check_polish(GameTuning& tuning, Presentation& presentation);

	// perf_checks.cpp
	void check_perf();
#endif
}
