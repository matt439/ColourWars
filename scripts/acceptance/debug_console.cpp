#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "game/debug/debug_director.h"
#include "game/debug/debug_tokenizer.h"
#include "game/presentation.h"
#include "game/tuning.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_clock.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_screenshot.h"
#include "game/objects/level_loaded_info.h"
#include <chrono>
#include <regex>
#include "game/debug/debug_log.h"
#include "game/debug/debug_registry.h"
#include "game/debug/debug_text_field.h"
#include "game/debug/match_plan.h"
#include "game/game_data.h"
#include "game/input_slots.h"
#include <algorithm>
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/core/registry.h"
#include "engine/input/gamepad_reader.h"
#include "engine/input/gamepads.h"
#include "engine/input/mouse.h"
#include "engine/collision/partitioner.h"
#include "engine/render/render_resources.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/sprite_font_file.h"
#include "engine/render/viewport_manager.h"
#include "engine/render/text_encoding.h"
#include "game/debug/bot.h"
#include "game/debug/bot_driver.h"
#include "game/debug/json_spans.h"
#include "game/debug/match_fingerprint.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "game/directory_consts.h"
#include "game/objects/level.h"
#include "game/objects/level_builder.h"
#include "game/source_paths.h"
#include "game/states/pause_menu.h"
#include <bit>
#include <cmath>
#include <cstdlib>
#include <map>
#include <process.h>
#else
#include "game/input_slots.h"
#include <io.h>
#include <process.h>
#endif

// The developer debug system's CPU-side parts: parsing, the registry, the
// debug clock, the match planner and the line editor; phase 2's chord, debug
// menu model, unwinding, overlays, cheats, goto, screenshot file and level
// stations; phase 3's JSON spans, live tuning on scratch copies of the content
// files, bots, the seeded planner, the replay file, and record and play on a
// real Level built without a device; in Release, that the build is inert and
// the executable carries none of it. Creates no window, device or save (the
// PNG check, the tuning and replay checks write and remove temporary files;
// the real content files are only read, and checked unchanged).
namespace
{
	void require(bool condition, const std::string& message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	std::string executable_text(const std::filesystem::path& game_directory)
	{
		std::ifstream stream(game_directory / L"ColourWars.exe", std::ios::binary);
		require(stream.good(), "Could not read ColourWars.exe");
		return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
	}

	// MSVC RTTI type names exist only for polymorphic classes that were compiled
	// and referenced, so they show whether the console code was linked at all.
	const char* const rtti_names[] = {
		".?AVDebugConsoleState@@", ".?AVDebugRegistry@@", ".?AVLambdaCommand@@",
		".?AVDebugMenuPage@@", ".?AVNumberVariable@@",
		// Phase 3: bots, record/play, live tuning, and the seat filter they share.
		".?AVBot@@", ".?AVPaintBot@@", ".?AVBotDriver@@", ".?AVReplayRecorder@@",
		".?AVReplayPlayer@@", ".?AVTuneTarget@@", ".?AVTuningTarget@@",
		".?AVSeatInputFilter@@",
	};

	void check_executable(const std::filesystem::path& game_directory)
	{
		const std::string text = executable_text(game_directory);
		for (const char* name : rtti_names)
		{
			const bool present = text.find(name) != std::string::npos;
			require(present == dev_tools::enabled, std::string(name) +
				(dev_tools::enabled ? " is missing from the Debug executable" :
					" leaked into the Release executable"));
		}
	}

#if !COLOURWARS_DEV_TOOLS
	// What `body` writes to stderr (the C stream), with \r\n read back as \n.
	template <typename Body>
	std::string captured_stderr(Body body)
	{
		const std::filesystem::path path = std::filesystem::temp_directory_path() /
			(L"colourwars_debug_console_" + std::to_wstring(_getpid()) + L".txt");
		std::fflush(stderr);
		const int saved = _dup(_fileno(stderr));
		require(saved != -1, "Could not duplicate stderr");
		FILE* file = nullptr;
		if (_wfopen_s(&file, path.c_str(), L"wb") != 0 || file == nullptr)
		{
			_close(saved);
			throw std::runtime_error("Could not open the stderr capture file");
		}
		_dup2(_fileno(file), _fileno(stderr));
		body();
		std::fflush(stderr);
		_dup2(saved, _fileno(stderr));
		_close(saved);
		std::fclose(file);

		std::string text;
		{
			std::ifstream stream(path, std::ios::binary);
			text.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		}
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		std::erase(text, '\r');
		return text;
	}
#endif

	void check_launch_detection()
	{
		require(has_launch_statements({ L"+stage", L"movement_course" }),
			"+stage must be detected as a launch statement");
		require(!has_launch_statements({ L"-flag", L"+0.5", L"+" }),
			"Only '+' followed by a letter starts a statement");
		require(!has_launch_statements({}), "No arguments, no statements");
	}

#if COLOURWARS_DEV_TOOLS
	void check_tokenizer()
	{
		DebugTokens tokens = tokenize("  set   feedback\toff ");
		require(tokens.ok && tokens.words == std::vector<std::string>{ "set", "feedback", "off" },
			"Whitespace separates words");
		tokens = tokenize("logfile \"C:/a b/c.log\" x");
		require(tokens.ok && tokens.words.size() == 3 && tokens.words[1] == "C:/a b/c.log",
			"Quotes group words");
		tokens = tokenize("echo \"say \\\"hi\\\"\"");
		require(tokens.ok && tokens.words.size() == 2 && tokens.words[1] == "say \"hi\"",
			"\\\" inside quotes is a literal quote");
		require(!tokenize("stage \"open").ok, "An unterminated quote is an error");
		require(!tokenize("stage caf\xc3\xa9").ok, "Non-ASCII input is rejected");
		require(tokenize("").words.empty(), "An empty line has no words");

		const std::vector<std::string> statements =
			split_statements("pause; step 3 ;; logfile \"a;b\"");
		require(statements.size() == 3 && statements[2] == " logfile \"a;b\"",
			"';' splits statements outside quotes and drops empty ones");
	}

	void check_launch_arguments()
	{
		std::vector<std::string> statements = parse_launch_arguments(
			{ L"+stage", L"movement_course", L"+players", L"4", L"+visual", L"posed" });
		require(statements == std::vector<std::string>{
			"stage movement_course", "players 4", "visual posed" },
			"Each +name starts a statement with the arguments after it");

		statements = parse_launch_arguments({ L"+timescale", L"+0.5" });
		require(statements == std::vector<std::string>{ "timescale +0.5" },
			"+ followed by a digit is an argument, not a statement");

		statements = parse_launch_arguments({ L"-future", L"x", L"+pause" });
		require(statements == std::vector<std::string>{ "pause" },
			"Tokens before the first statement are ignored");

		require(parse_launch_arguments({}).empty(), "No arguments, no statements");

		statements = parse_launch_arguments({ L"+logfile", L"C:\\my logs\\cw.log" });
		require(statements.size() == 1 &&
			tokenize(statements[0]).words.at(1) == "C:\\my logs\\cw.log",
			"An argument with spaces survives into the statement");
	}

	struct FakeGame
	{
		bool feedback = true;
		std::string visual = "original";
		int status_runs = 0;
	};

	void build_registry(DebugRegistry& registry, FakeGame& game)
	{
		registry.add(std::make_unique<BoolVariable>("feedback", "bursts", true,
			[&game] { return game.feedback; },
			[&game](bool value) { game.feedback = value; return DebugStatus::success(); }));
		registry.add(std::make_unique<ChoiceVariable>("visual", "art", "original",
			std::vector<std::string>{ "original", "rotating", "posed" },
			std::vector<std::pair<std::string, std::string>>{
				{ "continuous", "rotating" }, { "authored", "posed" } },
			[&game] { return game.visual; },
			[&game](const std::string& value) { game.visual = value; return DebugStatus::success(); }));
		registry.add(std::make_unique<LambdaCommand>("stage", "pick a stage",
			std::vector<DebugArgSpec>{ { "name", false, [] {
				return std::vector<std::string>{ "movement_course", "close_quarters",
					"next", "prev", "random" }; } } },
			[](const DebugArgs& args, DebugLog&) { return DebugStatus::success("stage " + args.text(0), true); }));
		registry.add(std::make_unique<LambdaCommand>("status", "what runs",
			std::vector<DebugArgSpec>{},
			[&game](const DebugArgs&, DebugLog&) { ++game.status_runs; return DebugStatus::success(); }));
		registry.add(std::make_unique<LambdaCommand>("step", "step",
			std::vector<DebugArgSpec>{ { "n", true, {} } },
			[](const DebugArgs&, DebugLog&) -> DebugStatus { throw std::runtime_error("boom"); }));
	}

	bool log_contains(const DebugLog& log, const std::string& text, DebugSeverity severity)
	{
		for (const DebugLogLine& line : log.lines())
		{
			if (line.severity == severity && line.text.find(text) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	void check_registry()
	{
		DebugRegistry registry;
		FakeGame game;
		build_registry(registry, game);
		DebugLog log;
		log.set_mirror_to_debugger(false);

		bool duplicate = false;
		try
		{
			registry.add(std::make_unique<LambdaCommand>("status", "again",
				std::vector<DebugArgSpec>{}, [](const DebugArgs&, DebugLog&) { return DebugStatus::success(); }));
		}
		catch (const std::logic_error&)
		{
			duplicate = true;
		}
		require(duplicate, "A duplicate name is rejected");

		require(registry.execute("feedback off", log).ok && !game.feedback,
			"Shorthand assigns a variable");
		require(registry.execute("feedback", log).ok && log_contains(log, "feedback = off", DebugSeverity::info),
			"Shorthand without a value reads it");
		require(registry.execute("set feedback toggle", log).ok && game.feedback,
			"set toggles a bool");
		require(registry.execute("get visual", log).ok &&
			log_contains(log, "visual = original  (choice, default original)", DebugSeverity::info),
			"get prints value, type and default");
		require(registry.execute("visual authored", log).ok && game.visual == "posed",
			"Aliases map onto their choice");
		require(registry.execute("visual next", log).ok && game.visual == "original",
			"next wraps around the choices");
		require(registry.execute("visual prev", log).ok && game.visual == "posed",
			"prev wraps around the choices");
		require(!registry.execute("visual sideways", log).ok && game.visual == "posed",
			"An unknown choice fails and changes nothing");
		require(!registry.execute("feedback maybe", log).ok, "A bad bool fails");

		require(!registry.execute("stage", log).ok &&
			log_contains(log, "usage: stage <name>", DebugSeverity::error),
			"A missing argument prints the usage line");
		require(!registry.execute("status now", log).ok, "An extra argument prints usage");
		require(!registry.execute("nope", log).ok && log_contains(log, "unknown command 'nope'",
			DebugSeverity::error), "An unknown name is an error");

		const DebugStatus thrown = registry.execute("step", log);
		require(!thrown.ok && log_contains(log, "exception: boom", DebugSeverity::error),
			"A throwing command is logged and does not propagate");

		const DebugStatus combined = registry.execute("status; stage movement_course; status", log);
		require(combined.ok && combined.close_console && game.status_runs == 2,
			"Statements run in order and close_console is OR-ed");

		require(registry.execute("list", log).ok && log_contains(log, "  feedback = on", DebugSeverity::info),
			"list prints variables with values");
		require(registry.execute("help stage", log).ok && log_contains(log, "stage <name>", DebugSeverity::info),
			"help describes one entry");

		// Completion.
		DebugCompletion completion = registry.complete("st");
		require(completion.line == "st" && completion.candidates ==
			std::vector<std::string>{ "stage", "status", "step" },
			"An ambiguous prefix lists candidates and keeps the common prefix");
		require(registry.complete("stage mo").line == "stage movement_course ",
			"A command argument completes from its choices");
		require(registry.complete("visual p").line == "visual posed ",
			"A choice variable completes its value");
		require(registry.complete("set feed").line == "set feedback ",
			"set completes a variable name");
		require(registry.complete("set visual ro").line == "set visual rotating ",
			"set completes the variable's value");
		require(registry.complete("pause; sta").line == "pause; sta" &&
			registry.complete("pause; stag").line == "pause; stage ",
			"Completion works on the last statement");
		require(registry.complete("zzz").line == "zzz" && registry.complete("zzz").candidates.empty(),
			"No match leaves the line alone");
	}

	void check_clock()
	{
		DebugClock clock;
		for (int i = 0; i < 5; ++i)
		{
			require(clock.updates_this_step() == 1, "Timescale 1 runs one update per step");
		}

		clock.set_timescale(0.25f);
		std::vector<int> runs;
		for (int i = 0; i < 8; ++i)
		{
			runs.push_back(clock.updates_this_step());
		}
		require(runs == std::vector<int>{ 0, 0, 0, 1, 0, 0, 0, 1 },
			"Timescale 0.25 runs one update every fourth step");

		clock.set_timescale(4.0f);
		require(clock.updates_this_step() == 4, "Timescale 4 runs four updates per step");
		clock.set_timescale(100.0f);
		require(clock.timescale() == DebugClock::max_timescale, "Timescale is clamped");
		clock.set_timescale(1.0f);

		std::vector<PlayerInputData> inputs(input_slots::count);
		inputs[4].jump_pressed = true;
		inputs[4].jump_held = true;
		inputs[4].toggle_debug = true;
		inputs[4].toggle_pause_menu = true;
		inputs[4].x_movement = 1.0f;
		const std::vector<PlayerInputData> later = DebugClock::without_edges(inputs);
		require(!later[4].jump_pressed && !later[4].toggle_debug,
			"Later runs of a step drop the latched press edges");
		require(later[4].jump_held && later[4].x_movement == 1.0f && later[4].toggle_pause_menu,
			"Later runs keep held input; the pause key is not the clock's");

		clock.set_paused(true);
		clock.request_steps(3);
		runs.clear();
		for (int i = 0; i < 4; ++i)
		{
			runs.push_back(clock.updates_this_step());
		}
		require(runs == std::vector<int>{ 1, 1, 1, 0 }, "Paused with step 3 runs exactly three updates");
		require(clock.paused(), "Stepping leaves the clock paused");
		clock.set_paused(false);
		clock.set_timescale(0.25f);
		require(!clock.is_default(), "A non-1 timescale is not default");
	}

	void check_planner()
	{
		StageList stages = {
			{ "movement_course", "Movement Course", "icon", false },
			{ "x", "X", "icon", true },
		};
		const MatchPlanner planner(&stages);
		const MatchPlanner::ConnectedPads no_pads{};
		unsigned int seed = 7;
		const MatchPlanner::Random random = [&seed] { return seed++; };

		MatchRequest request;
		request.stage = "movement_course";
		request.players = 2;
		request.visual = PlayerVisualMode::authored;
		MatchPlan plan = planner.resolve(nullptr, request, no_pads, random);
		require(plan.ok, "A stage and player count resolve: " + plan.error);
		const std::vector<MenuPlayerSettings> seats = plan.settings.player_settings();
		require(seats.size() == 2 && seats[0].num == input_slots::keyboard_and_mouse &&
			seats[1].num == -1, "With no pads, seat 0 is keyboard/mouse and seat 1 an idle dummy");
		require(seats[0].team == PlayerTeam::a && seats[1].team == PlayerTeam::b,
			"Teams alternate a, b");
		require(plan.settings.screen_layout() == labrador::ScreenLayout::two_player,
			"The layout follows the player count");
		require(plan.settings.workshop_visual_mode() == PlayerVisualMode::authored,
			"The visual round-trips through MenuLevelSettings");
		require(plan.reset_session, "A new roster starts a new session");
		for (const MenuPlayerSettings& seat : seats)
		{
			require(!input_slots::is_pad(seat.num),
				"No seat claims a pad that is not connected (that would open the pause menu)");
		}

		const MatchPlanner::ConnectedPads two_pads{ true, false, true, false };
		request.players = 4;
		plan = planner.resolve(nullptr, request, two_pads, random);
		require(plan.ok && plan.settings.player_settings()[0].num == 0 &&
			plan.settings.player_settings()[1].num == 2 &&
			plan.settings.player_settings()[2].num == input_slots::keyboard_and_mouse &&
			plan.settings.player_settings()[3].num == -1,
			"Connected pads in slot order, then keyboard/mouse, then dummies");

		MatchRequest weapons;
		weapons.weapons.push_back({ -1, "random" });
		weapons.weapons.push_back({ 1, "roller" });
		plan = planner.resolve(&plan.settings, weapons, two_pads, random);
		require(plan.ok, "Weapons resolve on a running match: " + plan.error);
		for (const MenuPlayerSettings& seat : plan.settings.player_settings())
		{
			require(seat.weapon != WeaponType::random_primary &&
				static_cast<int>(seat.weapon) < static_cast<int>(WeaponType::max_prim_wep),
				"weapon all random resolves to a concrete primary");
		}
		require(plan.settings.player_settings()[1].weapon == WeaponType::roller,
			"Later weapon choices override earlier ones");
		require(!plan.reset_session, "Weapons alone keep the session");

		MatchRequest next;
		next.stage = "next";
		const MatchPlan moved = planner.resolve(&plan.settings, next, two_pads, random);
		require(moved.ok && moved.settings.stage() == "x" && moved.reset_session,
			"next moves to the following stage");
		next.stage = "random";
		require(planner.resolve(&plan.settings, next, two_pads, random).settings.stage() == "x",
			"random picks only stages included in random");

		MatchRequest unknown;
		unknown.stage = "nowhere";
		require(!planner.resolve(nullptr, unknown, no_pads, random).ok, "An unknown stage fails");
		MatchRequest bad_seat;
		bad_seat.stage = "x";
		bad_seat.weapons.push_back({ 3, "sniper" });
		require(!planner.resolve(nullptr, bad_seat, no_pads, random).ok,
			"A weapon for a seat outside the roster fails");

		MatchRequest restart;
		restart.restart = true;
		const MatchPlan same = planner.resolve(&plan.settings, restart, two_pads, random);
		require(same.ok && !same.reset_session && same.settings.player_count() == 4,
			"A restart keeps the roster and the session");
		require(planner.is_stage_word("movement_course") && planner.is_stage_word("random") &&
			!planner.is_stage_word("movement"), "Stage words are checked when typed");
	}

	void check_text_field()
	{
		DebugTextField field;
		DebugKeyEvents typing;
		typing.typed = "stagx";
		field.apply(typing);
		DebugKeyEvents edit;
		edit.backspace = true;
		field.apply(edit);
		field.apply(DebugKeyEvents{ "e" });
		require(field.text() == "stage" && field.caret() == 5, "Typing and backspace edit at the caret");

		DebugKeyEvents left;
		left.left = true;
		field.apply(left);
		field.apply(left);
		field.apply(DebugKeyEvents{ "X" });
		require(field.text() == "staXge" && field.caret() == 4, "Left moves the caret; text inserts there");
		DebugKeyEvents del;
		del.left = true;
		field.apply(del);
		DebugKeyEvents erase;
		erase.del = true;
		field.apply(erase);
		require(field.text() == "stage", "Delete removes the character after the caret");
		DebugKeyEvents end;
		end.end = true;
		field.apply(end);
		field.apply(DebugKeyEvents{ " x\x01\xc3" });
		require(field.text() == "stage x", "Only printable ASCII is inserted");

		DebugKeyEvents clear;
		clear.clear_line = true;
		field.apply(clear);
		require(field.text().empty() && field.caret() == 0, "Ctrl+U clears the line");

		for (const char* line : { "status", "status", "pause", "  " })
		{
			field.set_text(line);
			field.submit();
		}
		require(field.history() == std::vector<std::string>{ "status", "pause" },
			"History skips repeats and blank lines");

		field.apply(DebugKeyEvents{ "draft" });
		DebugKeyEvents up;
		up.up = true;
		DebugKeyEvents down;
		down.down = true;
		field.apply(up);
		require(field.text() == "pause", "Up recalls the most recent line");
		field.apply(up);
		require(field.text() == "status", "Up again recalls the one before");
		field.apply(up);
		require(field.text() == "status", "Up stops at the oldest line");
		field.apply(down);
		field.apply(down);
		require(field.text() == "draft", "Down returns to the unsent draft");

		DebugKeyRepeat repeat;
		require(repeat.fire(true, true, 0.0f), "A press fires at once");
		int fired = 0;
		float time = 0.0f;
		const float frame = 1.0f / 60.0f;
		while (time + frame < DebugKeyRepeat::delay - 0.001f)
		{
			time += frame;
			fired += repeat.fire(true, false, frame) ? 1 : 0;
		}
		require(fired == 0, "No repeat before the delay");
		for (int i = 0; i < 60; ++i)
		{
			fired += repeat.fire(true, false, frame) ? 1 : 0;
		}
		require(fired >= 28 && fired <= 31, "About thirty repeats a second after the delay, got " +
			std::to_string(fired));
		require(!repeat.fire(false, false, frame), "Release stops the repeat");
	}

#endif

	// A match as the director sees it: counts updates and rebuilds, records
	// each update's jump edge, and can finish after a number of updates.
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
		int rebuilds = 0;
		bool last_reset_session = false;
		int leaves = 0;
		int audio_syncs = 0;

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
			this->last_filter = seats;
			this->filtered_updates += seats != nullptr ? 1 : 0;
		}
		bool debug_level_finished() const override
		{
			return this->finish_after >= 0 && this->updates >= this->finish_after;
		}
		const Level* debug_level() const override { return nullptr; }
		void debug_sync_audio() override { ++this->audio_syncs; }
		std::vector<std::string> debug_status() const override { return {}; }

		int jumps_delivered() const
		{
			return static_cast<int>(std::count(this->jumps.begin(), this->jumps.end(), true));
		}
		void reset_counts()
		{
			this->updates = 0;
			this->jumps.clear();
			this->finish_after = -1;
		}

	private:
		DebugDirector& director_;
	};

#if COLOURWARS_DEV_TOOLS

	// The director's match hooks, driven against a fake match: what the real
	// update_match, before_match_update, before_level_build, attach_match and
	// detach_match do, not a copy of their loops.
	void check_match_hooks(GameTuning& tuning, Presentation& presentation)
	{
		const StageList stages = {
			{ "movement_course", "Movement Course", "icon", false },
			{ "x", "X", "icon", true },
		};
		GameData data;
		data.set_stages(&stages);
		DebugDirector director(&tuning, &presentation);
		director.attach(nullptr, &data);
		DebugDirectorImpl& impl = *director.impl();
		impl.log().set_mirror_to_debugger(false);

		const float dt = 1.0f / 60.0f;
		FakeMatch match(director);
		MatchRequest roster;
		roster.stage = "x";
		roster.players = 2;
		unsigned int seed = 3;
		const MatchPlan started = MatchPlanner(&stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [&seed] { return seed++; });
		require(started.ok, "The fake match's roster resolves: " + started.error);
		match.settings = started.settings;

		// Settings stored in the menus without `stage` never reach a match
		// started another way.
		require(impl.execute("players 4").ok && impl.execute("weapon all sniper").ok &&
			impl.execute("visual posed").ok, "Menu-time settings are accepted");
		require(!impl.request().visual, "visual in the menus is remembered, not queued");
		director.attach_match(match);
		require(impl.request().empty(), "attach_match drops menu-time leftovers");
		require(log_contains(impl.log(), "dropped queued players 4", DebugSeverity::warning),
			"Dropping a leftover request is logged");
		require(!director.before_match_update(match, dt) && match.rebuilds == 0 &&
			match.settings.player_count() == 2,
			"A match started from the menus is not rebuilt from menu-time settings");

		const std::size_t kbm = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
		std::vector<PlayerInputData> tap(input_slots::count);
		tap[kbm].jump_pressed = true;
		tap[kbm].jump_held = true;
		const std::vector<PlayerInputData> none(input_slots::count);

		// Timescale 1: one update, the edge delivered.
		director.update_match(match, tap, dt);
		require(match.updates == 1 && match.jumps_delivered() == 1,
			"Timescale 1 runs one update with the press");

		// Slow motion: a press on a step with no update is delivered once, later.
		match.reset_counts();
		impl.execute("timescale 0.25");
		impl.clock().reset_accumulator();
		for (int step = 0; step < 8; ++step)
		{
			director.update_match(match, step == 0 ? tap : none, dt);
		}
		require(match.updates == 2, "Timescale 0.25 runs two updates in eight steps, got " +
			std::to_string(match.updates));
		require(match.jumps_delivered() == 1 && match.jumps.front(),
			"A press during slow motion is delivered exactly once, on the next update");

		// Fast forward: four updates, the edge only on the first.
		match.reset_counts();
		impl.execute("timescale 4");
		director.update_match(match, tap, dt);
		require(match.updates == 4 && match.jumps_delivered() == 1 && match.jumps.front(),
			"Timescale 4 runs four updates and delivers the press once");

		// A round that ends mid fast-forward stops there.
		match.reset_counts();
		match.finish_after = 2;
		director.update_match(match, none, dt);
		require(match.updates == 2, "A level that finishes mid fast-forward is not updated again");
		match.reset_counts();
		impl.execute("timescale 1");

		// Paused: nothing runs; a step runs one update and resyncs audio.
		impl.execute("pause on");
		director.update_match(match, tap, dt);
		require(match.updates == 0, "A paused clock runs no update");
		const int syncs = match.audio_syncs;
		require(impl.execute("step 1").ok, "step works in a match");
		director.update_match(match, none, dt);
		require(match.updates == 1 && match.jumps_delivered() == 1,
			"A step delivers the press latched while paused");
		require(match.audio_syncs > syncs, "A stepped update resyncs the paused audio");
		impl.execute("pause off");

		// Restart keeps the roster and the session.
		require(impl.execute("restart").ok, "restart works in a match");
		require(director.before_match_update(match, dt) && match.rebuilds == 1 &&
			!match.last_reset_session && match.settings.player_count() == 2,
			"restart rebuilds once with the current settings and keeps the session");
		require(impl.request().empty(), "A rebuild consumes the request");

		// A roster change rebuilds with the planned settings.
		require(impl.execute("players 3").ok && impl.execute("weapon 2 roller").ok,
			"In a match, players and weapon queue a rebuild");
		require(director.before_match_update(match, dt) && match.rebuilds == 2 &&
			match.last_reset_session && match.settings.player_count() == 3 &&
			match.settings.player_weapon(1) == WeaponType::roller,
			"debug_rebuild receives the planned roster and weapons");

		// A restart from the pause or end menu is the restart: its build
		// consumes the queued one, so the match is not built twice.
		require(impl.execute("restart").ok, "restart queues");
		director.before_level_build(match);
		require(impl.request().empty() && !director.before_match_update(match, dt) &&
			match.rebuilds == 2, "A menu restart consumes a queued debug restart");

		// What was queued for a match that then ends does not carry over.
		require(impl.execute("players 4").ok, "players queues in a match");
		director.detach_match(match);
		require(impl.request().empty() &&
			log_contains(impl.log(), "dropped queued players 4: the match ended",
				DebugSeverity::warning), "detach_match drops a request queued for the match");
		require(!impl.in_match(), "detach_match forgets the match");
		require(match.leaves == 0, "Nothing asked the match to leave");
	}

	void check_director(GameTuning& tuning, Presentation& presentation)
	{
		DebugDirector director(&tuning, &presentation);
		require(director.enabled() && director.impl() != nullptr, "Debug builds have a director");
		require(!DebugDirector::inert().enabled(), "The inert director has no implementation");

		DebugDirectorImpl& impl = *director.impl();
		impl.log().set_mirror_to_debugger(false);
		const bool before = presentation.movement_feedback().enabled;
		require(impl.execute("feedback toggle").ok &&
			presentation.movement_feedback().enabled != before,
			"feedback edits the live presentation");
		impl.execute("feedback on");

		require(impl.execute("timescale 0.5").ok && impl.clock().timescale() == 0.5f,
			"timescale sets the clock");
		require(!impl.execute("timescale 9").ok && impl.clock().timescale() == 0.5f,
			"timescale outside 0.25-4 fails and changes nothing");
		require(impl.execute("pause on").ok && impl.clock().paused(), "pause on pauses");
		require(impl.execute("pause").ok && !impl.clock().paused(), "pause toggles by default");
		require(!impl.execute("step 2").ok, "step needs a match");
		require(!impl.execute("restart").ok, "restart needs a match");
		require(impl.execute("menu").ok, "menu in the menus is a no-op");

		require(impl.execute("players 4").ok && impl.request().players == 4,
			"players is stored as a request");
		require(impl.execute("visual posed").ok && impl.status_strip().find("visual posed") !=
			std::string::npos, "visual is stored for the next match and shown in the strip");
		require(!impl.execute("weapon 5 roller").ok && !impl.execute("weapon all spoon").ok,
			"weapon checks its seat and name");
		require(impl.execute("weapon all random").ok && impl.request().weapons.size() == 1,
			"weapon is stored as a request");
		require(!impl.execute("reload").ok, "reload needs the running game");
		require(impl.status_strip().find("x0.5") != std::string::npos,
			"The strip shows a non-default timescale");

		// Launch statements run through the same registry.
		director.run_launch_arguments({ L"+timescale", L"2", L"+feedback", L"off" });
		require(impl.clock().timescale() == 2.0f && !presentation.movement_feedback().enabled,
			"Launch arguments run as console statements");
		presentation.set_movement_feedback_enabled(before);
	}

	// ---------------------------------------------------------------- phase 2

	const StageList& test_stages()
	{
		static const StageList stages = {
			{ "movement_course", "Movement Course", "icon", false },
			{ "x", "X", "icon", true },
		};
		return stages;
	}

	// A match's settings with `players` seats on stage x (seat 0 keyboard/mouse).
	MenuLevelSettings planned(int players)
	{
		MatchRequest roster;
		roster.stage = "x";
		roster.players = players;
		unsigned int seed = 11;
		const MatchPlan plan = MatchPlanner(&test_stages()).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [&seed] { return seed++; });
		require(plan.ok, "The test roster resolves: " + plan.error);
		return plan.settings;
	}

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

	std::size_t row_index(const std::vector<DebugMenuRow>& rows, const std::string& label)
	{
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i].label == label) { return i; }
		}
		throw std::runtime_error("The debug menu has no row '" + label + "'");
	}

	std::vector<std::string> labels(const std::vector<DebugMenuRow>& rows)
	{
		std::vector<std::string> result;
		for (const DebugMenuRow& row : rows) { result.push_back(row.label); }
		return result;
	}
#endif

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
	void check_chord()
	{
		// 0.125 s frames are exact in binary: the fourth fires.
		DebugChord chord;
		DebugChord::Pads pads{};
		pads[0].connected = true;
		pads[1].connected = true;
		const float step = 0.125f;

		pads[0].back = true;
		require(!chord.update(pads, step) && !chord.suppressing(0), "Back alone starts no chord");
		pads[0].back = false;
		pads[0].start = true;
		require(!chord.update(pads, step) && !chord.suppressing(0), "Start alone starts no chord");

		pads[0].back = true;
		std::vector<bool> fired;
		for (int frame = 0; frame < 6; ++frame)
		{
			fired.push_back(chord.update(pads, step));
			require(chord.suppressing(0), "A held chord suppresses its pad");
		}
		require(fired == std::vector<bool>{ false, false, false, true, false, false },
			"The chord fires once, on the frame the hold reaches 0.5 s");
		require(!chord.suppressing(1) && chord.any_suppressing(), "Pads are independent");

		pads[0].back = false;
		require(!chord.update(pads, step) && chord.suppressing(0),
			"Releasing Back first: Start's release is still the chord's");
		pads[0].start = false;
		require(!chord.update(pads, step) && chord.suppressing(0),
			"The frame both are up is still suppressed");
		require(!chord.update(pads, step) && !chord.suppressing(0) && !chord.any_suppressing(),
			"The frame after, the pad is the game's again");

		// Aborted: released early, never fires, both edges still eaten.
		pads[0].back = pads[0].start = true;
		require(!chord.update(pads, step) && !chord.update(pads, step), "Two frames do not fire");
		pads[0].back = pads[0].start = false;
		require(!chord.update(pads, step) && chord.suppressing(0),
			"An aborted chord still suppresses its release frame");
		require(!chord.update(pads, step) && !chord.suppressing(0), "Then it is over");

		// A disconnect ends the session.
		pads[0].back = pads[0].start = true;
		chord.update(pads, step);
		pads[0].connected = false;
		chord.update(pads, step);
		pads[0].connected = true;
		pads[0].back = pads[0].start = false;
		require(!chord.update(pads, step) && !chord.suppressing(0), "A disconnect ends the chord");

		// At 60 Hz the thirtieth frame fires, whatever single-precision sums do.
		chord.reset();
		pads[0].back = pads[0].start = true;
		int fired_on = 0;
		for (int frame = 1; frame <= 40; ++frame)
		{
			if (chord.update(pads, 1.0f / 60.0f)) { fired_on = frame; }
		}
		require(fired_on == 30, "At 60 Hz the chord fires on frame 30, got " +
			std::to_string(fired_on));
	}

	void check_input_filter(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugChord::Pads pads{};
		pads[0].connected = true;
		pads[0].back = pads[0].start = true;
		test.impl.chord().update(pads, 1.0f / 60.0f);

		std::vector<PlayerInputData> inputs(input_slots::count);
		for (PlayerInputData& input : inputs)
		{
			input.toggle_debug = true;
			input.toggle_pause_menu = true;
		}
		test.director.filter_match_inputs(inputs);
		require(!inputs[0].toggle_debug && !inputs[0].toggle_pause_menu,
			"The chord's pad loses its Back and Start edges");
		require(inputs[1].toggle_debug && inputs[1].toggle_pause_menu,
			"Another pad keeps its edges");
		const std::size_t kbm = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
		require(inputs[kbm].toggle_debug && inputs[kbm].toggle_pause_menu,
			"The keyboard keeps F1 and Escape");
	}

	void check_registry_menu_traits(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		const DebugRegistry& registry = test.impl.registry();
		require(registry.categories() == std::vector<std::string>{
			"match", "player", "view", "time", "tuning", "replay", "content", "console" },
			"Categories come in the fixed order");
		for (const char* name : { "stage", "players", "weapon", "visual" })
		{
			require(registry.find(name)->menu().staged, std::string(name) + " is staged");
		}
		for (const char* name : { "restart", "goto", "god", "timescale", "screenshot" })
		{
			require(!registry.find(name)->menu().staged, std::string(name) + " is not staged");
		}
		for (const char* name : { "set", "get", "list", "help", "clear", "logfile" })
		{
			require(registry.find(name)->menu().form == DebugMenuTraits::Form::hidden,
				std::string(name) + " is console-only");
		}
		using Form = DebugMenuTraits::Form;
		const DebugMenuTraits& weapon = registry.find("weapon")->menu();
		require(weapon.form == Form::pick && weapon.pick_argument == 1 && weapon.prefix == "all",
			"weapon picks its second argument, for every seat");
		require(registry.find("stage")->menu().form == Form::pick &&
			registry.find("goto")->menu().form == Form::pick &&
			registry.find("overlay")->menu().form == Form::pick,
			"stage, goto and overlay are pickers");
		require(registry.find("restart")->menu().form == Form::run &&
			registry.find("respawn")->menu().form == Form::run,
			"restart and respawn run");
		require(registry.find("timescale")->menu().form == Form::variable &&
			registry.find("overlay.collision")->menu().category == "view",
			"Variables are variable rows in their category");

		DebugDirectorImpl& impl = test.impl;
		require(impl.execute("timescale next").ok && impl.clock().timescale() == 2.0f,
			"timescale next from 1 is 2");
		impl.execute("timescale 0.25");
		require(impl.execute("timescale prev").ok && impl.clock().timescale() == 0.25f,
			"timescale prev stops at 0.25");
		require(impl.execute("timescale 0.3").ok && impl.clock().timescale() == 0.3f,
			"Any timescale in range is accepted");
		require(!impl.execute("timescale 9").ok && impl.clock().timescale() == 0.3f &&
			log_contains(impl.log(), "timescale: expected 0.25-4, got '9'", DebugSeverity::error),
			"An out-of-range timescale fails with the range");
		require(impl.execute("timescale").ok && log_contains(impl.log(), "timescale = 0.3",
			DebugSeverity::info), "timescale alone prints it");

		require(impl.registry().variable("god")->step_word(1) == "toggle",
			"A bool steps by toggling");
		require(impl.execute("ammo next").ok && impl.registry().variable("ammo")->value_text() == "inf" &&
			impl.execute("ammo next").ok && impl.registry().variable("ammo")->value_text() == "normal",
			"A choice steps and wraps");
	}

	void check_menu_model(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		FakeMatch match(test.director);
		match.settings = planned(1);
		test.director.attach_match(match);
		const float dt = 1.0f / 60.0f;

		DebugMenuModel model(impl);
		require(labels(model.rows()) == std::vector<std::string>{
			"match", "player", "view", "time", "tuning", "replay", "content", "console" },
			"With nothing queued the root lists the categories");

		model.confirm(row_index(model.rows(), "match"));
		require(model.title() == "DEBUG / match", "The title names the category");
		require(labels(model.rows()) == std::vector<std::string>{
			"menu", "players", "restart", "stage", "visual", "weapon" },
			"A category lists its entries by name");
		model.confirm(row_index(model.rows(), "stage"));
		require(labels(model.rows()) == std::vector<std::string>{
			"movement_course", "x", "next", "prev", "random" }, "stage opens a picker");
		require(model.confirm(row_index(model.rows(), "x")) == DebugMenuModel::Result::stay &&
			impl.request().stage == "x" && match.rebuilds == 0,
			"A staged choice queues without rebuilding and stays open");
		require(log_contains(impl.log(), "[menu] stage x", DebugSeverity::echo),
			"The menu's statements are echoed");

		model.confirm(row_index(model.rows(), "weapon"));
		require(labels(model.rows()).size() == MatchPlanner::weapon_names().size(),
			"weapon picks among the weapon names");
		model.confirm(row_index(model.rows(), "roller"));
		require(impl.request().weapons.size() == 1 && impl.request().weapons[0].seat == -1 &&
			impl.request().weapons[0].weapon == "roller", "The picker builds weapon all roller");

		require(model.back() == DebugMenuModel::Result::stay, "Back from a category stays open");
		const std::vector<DebugMenuRow> root = model.rows();
		require(root[0].kind == DebugMenuRow::Kind::apply && root[0].enabled &&
			root[0].label == "Apply: stage x, weapon all roller, visual original",
			"Apply names what is queued, got '" + root[0].label + "'");
		require(root[1].kind == DebugMenuRow::Kind::discard, "Discard follows Apply");
		require(model.focus_hint() == 0, "Back after staging lands on Apply");
		require(model.confirm(0) == DebugMenuModel::Result::close, "Apply closes the menu");
		require(test.director.before_match_update(match, dt) && match.rebuilds == 1 &&
			match.settings.stage() == "x" && match.settings.player_weapon(0) == WeaponType::roller,
			"What was staged applies once, through the normal path");

		impl.execute("stage x");
		require(model.confirm(row_index(model.rows(), "Discard queued")) ==
			DebugMenuModel::Result::stay && impl.request().empty() &&
			log_contains(impl.log(), "discarded in the debug menu", DebugSeverity::warning),
			"Discard drops the request and says so");

		model.confirm(row_index(model.rows(), "time"));
		const std::size_t timescale = row_index(model.rows(), "timescale");
		require(model.rows()[timescale].value == "1", "A variable row shows its value");
		require(model.step(timescale, 1) == DebugMenuModel::Result::stay &&
			impl.clock().timescale() == 2.0f, "Right steps a number up");
		require(model.step(timescale, -1) == DebugMenuModel::Result::stay &&
			impl.clock().timescale() == 1.0f, "Left steps it down");
		require(model.confirm(row_index(model.rows(), "step")) == DebugMenuModel::Result::close,
			"A command that asks the console to close closes the menu");
		impl.execute("pause off");

		model.back();
		model.confirm(row_index(model.rows(), "view"));
		model.step(row_index(model.rows(), "overlay.collision"), 1);
		require(match.overlays.collision && !match.overlays.camera,
			"Stepping an overlay variable applies to the match at once");
		model.confirm(row_index(model.rows(), "overlay.collision"));
		require(!match.overlays.collision, "Confirm on a bool steps it too");
		require(model.back() == DebugMenuModel::Result::stay &&
			model.back() == DebugMenuModel::Result::close, "Back at the top closes");
		test.director.detach_match(match);
	}

	void check_unwinding(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		DebugDirector& director = test.director;
		const float dt = 1.0f / 60.0f;
		FakeMatch match(director);
		match.settings = planned(1);
		director.attach_match(match);

		FakePage page;
		require(!director.before_page_update(page, dt) && page.dismissals == 0,
			"Nothing queued: the page over the match is left alone");
		impl.execute("players 3");
		require(director.before_page_update(page, dt) && page.dismissals == 1 &&
			!page.last_leaving, "A queued rebuild dismisses the page over the match");
		require(impl.request().players == 3, "Dismissing keeps the request for the match");
		require(director.before_match_update(match, dt) && match.rebuilds == 1 &&
			match.settings.player_count() == 3, "Then the match applies it");

		impl.execute("menu");
		require(director.before_page_update(page, dt) && page.last_leaving,
			"menu dismisses the page toward the way out");
		require(director.before_match_update(match, dt) && match.leaves == 1,
			"and the match then leaves");

		FakePage stuck;
		stuck.dismiss_result = false;
		impl.execute("restart");
		require(!director.before_page_update(stuck, dt) &&
			log_contains(impl.log(), "applies when the match resumes", DebugSeverity::info),
			"A page with no way out keeps the request waiting, and says so");
		director.before_level_build(match);
		require(impl.request().empty(), "A menu build consumes a queued restart");

		// The adopt path: a menu's own restart builds once with the queued plan.
		const int rebuilds = match.rebuilds;
		impl.execute("players 2");
		director.before_level_build(match);
		require(match.adopts == 1 && match.settings.player_count() == 2 && match.last_reset_session &&
			impl.request().empty(), "A menu restart adopts the queued plan");
		require(log_contains(impl.log(), "(adopted by the menu's build)", DebugSeverity::info),
			"The adoption is logged");
		require(!director.before_match_update(match, dt) && match.rebuilds == rebuilds,
			"An adopted plan does not rebuild a second time");

		// The chord resets the page's arming on every frame it suppresses.
		DebugChord::Pads pads{};
		pads[2].connected = true;
		pads[2].back = pads[2].start = true;
		impl.chord().update(pads, dt);
		FakePage chorded;
		director.before_page_update(chorded, dt);
		require(chorded.resets == 1 && chorded.reset_slots == std::vector<int>{ 2 },
			"A chord in progress resets only its own pad's menu input");
		impl.chord().reset();

		// In the menus a queued stage starts from the page, or closes a page
		// that cannot start one.
		director.detach_match(match);
		impl.execute("stage x");
		FakePage options;
		options.start_result = false;
		require(director.before_page_update(options, dt) && options.starts == 1 &&
			options.dismissals == 1 && impl.request().stage == "x",
			"A page that cannot start the match is closed, and the stage stays queued");
		FakePage title;
		title.start_result = true;
		require(director.before_page_update(title, dt) && title.starts == 1 &&
			impl.request().empty(), "A main-menu page starts it");
		require(log_contains(impl.log(), "match: starting x", DebugSeverity::info),
			"The start is logged");
		director.attach_match(match);
		director.detach_match(match);
	}

	void check_overlays_and_cheats(GameTuning& tuning, Presentation& presentation)
	{
		{
			TestDirector test(tuning, presentation);
			DebugDirectorImpl& impl = test.impl;
			const float dt = 1.0f / 60.0f;
			FakeMatch match(test.director);
			match.settings = planned(2);
			test.director.attach_match(match);

			// A player's own F1 text survives the pane overlays.
			match.player_text = true;
			require(impl.execute("overlay collision").ok && match.overlays.collision &&
				!match.overlays.camera && match.player_text && match.player_text_sets == 0,
				"overlay <name> toggles one and leaves each player's own text alone");
			require(impl.execute("overlay.camera on").ok && impl.execute("overlay camera").ok &&
				impl.execute("overlay paint").ok && impl.execute("overlay paint").ok &&
				match.player_text && match.player_text_sets == 0,
				"overlay.camera, overlay camera and overlay paint push no player text");
			require(impl.execute("overlay.player off").ok && !match.player_text &&
				match.player_text_sets == 1, "overlay.player pushes the player text");
			require(impl.execute("overlay all").ok && match.overlays.collision &&
				match.overlays.camera && match.player_text, "overlay all turns every one on");
			require(impl.status_strip().find("ovl collision camera paint") != std::string::npos,
				"The strip names the overlays");
			const std::vector<std::string> paint = impl.paint_lines();
			require(paint.size() == 2 && paint[0] == "paint  A 30.0%  B 10.0%  free 60.0%" &&
				paint[1] == "P1 kbm 30.0%   P2 idle 0.0%",
				"overlay.paint shows team and seat coverage, got '" +
				(paint.empty() ? std::string() : paint[0]) + "'");
			require(impl.execute("overlay none").ok && !match.overlays.collision &&
				!match.overlays.camera && !match.player_text && impl.paint_lines().empty(),
				"overlay none clears them");
			require(impl.execute("overlay").ok && log_contains(impl.log(), "  overlay.paint = off",
				DebugSeverity::info), "overlay alone lists them");

			require(impl.execute("god 1").ok && impl.execute("ammo inf").ok &&
				match.damage_immune && match.infinite_ammo, "god and ammo reach the players at once");
			require(impl.status_strip().find("GOD") != std::string::npos &&
				impl.status_strip().find("AMMO INF") != std::string::npos,
				"The strip shows the cheats");

			impl.execute("overlay camera");
			match.damage_immune = match.infinite_ammo = false;
			match.overlays = LevelOverlays{};
			impl.execute("restart");
			require(test.director.before_match_update(match, dt) && match.damage_immune &&
				match.infinite_ammo && match.overlays.camera,
				"Every build re-applies the overlays and cheats");
			test.director.detach_match(match);
		}
		{
			// Launch statements run after attach and before any match.
			TestDirector test(tuning, presentation);
			test.director.run_launch_arguments({ L"+overlay", L"collision", L"+god", L"1" });
			FakeMatch match(test.director);
			match.settings = planned(1);
			test.director.attach_match(match);
			test.director.after_level_build(match);
			require(match.overlays.collision && match.damage_immune,
				"+overlay and +god apply at the first build");
			test.director.detach_match(match);
		}
	}

	void check_goto(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		DebugDirector& director = test.director;

		// The menus: no stage, a random stage and an unloaded stage name no
		// stations, and completion never throws.
		require(impl.station_names().empty(), "No match, no stage: no stations");
		impl.execute("stage random");
		require(impl.station_names().empty() && impl.registry().complete("goto ").candidates.empty(),
			"stage random names no stations yet");
		impl.execute("stage x");
		require(impl.registry().complete("goto h").line == "goto h",
			"A stage with no loaded level completes nothing");
		impl.drop_request("test");

		// Typed in the menus, a goto waits for the match `stage` starts...
		require(impl.execute("goto high_step").ok, "goto is stored before a match");
		FakeMatch match(director);
		match.settings = planned(1);
		director.attach_match(match);
		director.after_level_build(match);
		require(match.placed.empty() &&
			log_contains(impl.log(), "dropped queued goto high_step", DebugSeverity::warning),
			"...and a match started from the menus drops it");
		director.detach_match(match);

		impl.execute("goto high_step 1");
		impl.execute("stage x");
		FakePage title;
		title.start_result = true;
		director.before_page_update(title, 1.0f / 60.0f);
		director.attach_match(match);
		director.after_level_build(match);
		require(match.placed.size() == 1 && match.placed[0].first == 0 &&
			match.placed[0].second == mattmath::Vector2F(4404.0f, 535.0f),
			"A goto stored with `stage` applies at that match's first build");
		match.placed.clear();

		require(impl.execute("goto high_step").ok && match.placed.size() == 1 &&
			log_contains(impl.log(), "goto high_step: P1 at (4404, 535)", DebugSeverity::info),
			"In a match goto places seat 1 at once");
		require(!impl.execute("goto high_step 2").ok &&
			log_contains(impl.log(), "goto: seat 2 is not in this match (1 player)", DebugSeverity::error),
			"A seat outside the match fails and names the count");
		require(!impl.execute("goto nowhere").ok &&
			log_contains(impl.log(), "one of: high_step spawn_a", DebugSeverity::error),
			"An unknown station fails and lists the stations");
		require(impl.registry().complete("goto hi").line == "goto high_step ",
			"goto completes from the live stations");

		require(impl.execute("respawn all").ok && match.respawned == std::vector<int>{ 0 },
			"respawn all respawns every seat");

		// With a rebuild queued, goto waits for it (which would otherwise put
		// the player back at a spawn) and respawn says why it does nothing.
		match.placed.clear();
		impl.execute("restart");
		const DebugStatus waiting = impl.execute("goto high_step");
		require(waiting.ok && !waiting.close_console && match.placed.empty() &&
			log_contains(impl.log(), "goto high_step: applies after the queued restart",
				DebugSeverity::info), "goto waits for a queued rebuild");
		require(!impl.execute("respawn").ok && match.respawned.size() == 1 &&
			log_contains(impl.log(), "respawn: the queued restart respawns every seat anyway",
				DebugSeverity::error), "respawn is refused while a rebuild is queued");
		require(director.before_match_update(match, 1.0f / 60.0f) && match.placed.size() == 1 &&
			match.placed[0].second == mattmath::Vector2F(4404.0f, 535.0f),
			"The queued goto applies after the rebuild");
		match.placed.clear();
		impl.execute("restart");
		impl.execute("goto high_step");
		director.detach_match(match);
		require(log_contains(impl.log(), "dropped queued goto high_step: the match ended",
			DebugSeverity::warning), "A goto waiting for a rebuild is dropped with the match");
		director.attach_match(match);
		director.after_level_build(match);
		require(match.placed.empty(), "...and never reaches the next match");

		// A queued concrete stage names its stations, not the running match's.
		impl.execute("stage x");
		require(impl.registry().complete("goto hi").line == "goto hi",
			"With a stage queued, goto completes from that stage, not the current one");
		impl.drop_request("test");
		require(!impl.execute("respawn 3").ok, "respawn checks the seat");
		director.detach_match(match);
		require(!impl.execute("respawn").ok && log_contains(impl.log(), "respawn: no match",
			DebugSeverity::error), "respawn needs a match");
	}

	void check_screenshot_file(GameTuning& tuning, Presentation& presentation)
	{
		const auto now = std::chrono::system_clock::now();
		require(screenshot_file_name("my shot!", now) == "my_shot_.png",
			"A name keeps [A-Za-z0-9_-]");
		require(screenshot_file_name("drive.png", now) == "drive.png",
			"A trailing .png is not doubled");
		require(std::regex_match(screenshot_file_name("", now),
			std::regex("colourwars-[0-9]{8}-[0-9]{6}-[0-9]{3}\\.png")),
			"The default name is a millisecond timestamp, got " + screenshot_file_name("", now));

		const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		require(SUCCEEDED(com) || com == RPC_E_CHANGED_MODE, "COM initialises");
		ScreenCapture image;
		image.width = 3;
		image.height = 2;
		for (int i = 0; i < 6; ++i)
		{
			image.rgba.insert(image.rgba.end(), { static_cast<unsigned char>(i * 40),
				static_cast<unsigned char>(255 - i * 40), 7, 255 });
		}
		const std::filesystem::path path = std::filesystem::temp_directory_path() /
			L"colourwars_debug_console_shot.png";
		std::string error;
		require(write_png(path, image, error), "write_png writes a PNG: " + error);
		std::string bytes;
		{
			std::ifstream stream(path, std::ios::binary);
			bytes.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		}
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		const auto byte = [&bytes](std::size_t i) { return static_cast<unsigned char>(bytes.at(i)); };
		require(bytes.size() > 24 && bytes.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0,
			"The file starts with the PNG signature");
		const unsigned int width = (byte(16) << 24) | (byte(17) << 16) | (byte(18) << 8) | byte(19);
		const unsigned int height = (byte(20) << 24) | (byte(21) << 16) | (byte(22) << 8) | byte(23);
		require(width == 3 && height == 2, "IHDR carries the capture's size");
		ScreenCapture wrong;
		wrong.width = 2;
		require(!write_png(path, wrong, error), "A malformed capture is refused");
		if (SUCCEEDED(com))
		{
			CoUninitialize();
		}

		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		const DebugStatus queued = impl.execute("screenshot drive");
		require(queued.ok && queued.close_console && impl.pending_screenshot() == "drive",
			"screenshot queues and closes the console so the shot is clean");

		// The shot waits only for something that will apply next frame.
		require(!impl.screenshot_waits(), "Nothing queued: the shot is taken");
		impl.execute("players 2");
		impl.execute("weapon all roller");
		require(!impl.screenshot_waits(),
			"In the menus, players or weapon without a stage never hold the shot");
		impl.execute("stage x");
		require(impl.screenshot_waits(), "In the menus, a queued stage holds it until the match");
		impl.drop_request("test");
		FakeMatch match(test.director);
		match.settings = planned(1);
		test.director.attach_match(match);
		require(!impl.screenshot_waits(), "A quiet match does not hold it");
		impl.execute("restart");
		require(impl.screenshot_waits(), "In a match, a queued rebuild holds it");
		test.director.before_match_update(match, 1.0f / 60.0f);
		require(!impl.screenshot_waits(), "Once the rebuild applied, it is taken");
		impl.execute("menu");
		require(impl.screenshot_waits(), "A queued leave holds it");
		test.director.detach_match(match);
	}

	void check_stations(const std::filesystem::path& game_directory, const GameTuning& tuning)
	{
		const std::filesystem::path course = game_directory / L"levels" / L"movement_course.json";
		LevelLoadedInfo info(course.string().c_str());
		info.validate();
		const std::vector<LevelStation> stations = info.stations();
		std::vector<std::string> names;
		for (const LevelStation& station : stations) { names.push_back(station.name); }
		require(names == std::vector<std::string>{ "drop_through", "short_gap", "long_gap",
			"running_gap", "low_ceiling", "steps", "high_step", "recovery_lane",
			"spawn_a", "spawn_b" }, "The course lists its eight stations, then the spawns");
		require(stations[8].position == info.team_a_spawns()[0] &&
			stations[9].position == info.team_b_spawns()[0], "spawn_a and spawn_b are the first spawns");

		struct Surface { std::string name; mattmath::RectangleF rectangle; bool jump_through; };
		std::vector<Surface> surfaces;
		const labrador::JsonValue objects = info.collision_objects_json();
		for (std::size_t index = 0; index < objects.size(); ++index)
		{
			const labrador::JsonValue object = objects.at(index);
			const labrador::JsonValue shape = object.object("rectangle");
			surfaces.push_back({ object.string("name"), mattmath::RectangleF(shape.number("x"),
				shape.number("y"), shape.number("width"), shape.number("height")),
				object.string("collision_type") == "STRUCTURE_JUMP_THROUGH" });
		}
		const mattmath::Vector2F size = tuning.player().size;
		const mattmath::RectangleF bounds = info.out_of_bounds_rectangle();
		for (std::size_t i = 0; i < 8; ++i)
		{
			const mattmath::RectangleF body(stations[i].position, size);
			require(bounds.contains(body), stations[i].name + " is inside the level");
			bool supported = false;
			for (const Surface& surface : surfaces)
			{
				const mattmath::RectangleF& r = surface.rectangle;
				require(surface.jump_through || !r.intersects(body),
					stations[i].name + " overlaps " + surface.name);
				const float drop = r.top() - body.bottom();
				const float centre = body.center().x;
				supported = supported || (drop >= 0.0f && drop <= 10.0f &&
					centre >= r.left() && centre <= r.right());
			}
			require(supported, stations[i].name + " has a surface just under it");
		}

		// Bad stations are content bugs that fail at startup.
		std::string text;
		{
			std::ifstream stream(course, std::ios::binary);
			text.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		}
		const std::size_t at = text.find("\"stations\": [");
		require(at != std::string::npos, "The course has a stations array");
		const std::size_t insert = at + std::string("\"stations\": [").size();
		const std::filesystem::path bad = std::filesystem::temp_directory_path() /
			L"colourwars_bad_stations.json";
		const auto write_bad = [&bad](const std::string& content)
			{
				std::ofstream stream(bad, std::ios::binary | std::ios::trunc);
				stream << content;
				stream.close();
				require(!stream.fail(), "Could not write the temporary level");
			};
		// The control: the course itself, copied to the same path, validates,
		// so each failure below is its own rule's.
		write_bad(text);
		LevelLoadedInfo(bad.string().c_str()).validate();
		struct BadStation { const char* entry; const char* phrase; };
		for (const BadStation& candidate : {
			BadStation{ "{ \"name\": \"steps\", \"x\": 3900.0, \"y\": 875.0 },",
				"repeats the station name" },
			BadStation{ "{ \"name\": \"spawn_a\", \"x\": 100.0, \"y\": 875.0 },",
				"which every level already has" },
			BadStation{ "{ \"name\": \"far_away\", \"x\": -90000.0, \"y\": 875.0 },",
				"outside its own out_of_bounds" },
			BadStation{ "{ \"name\": \"High\", \"x\": 100.0, \"y\": 875.0 },",
				"lower-case letters" } })
		{
			write_bad(text.substr(0, insert) + candidate.entry + text.substr(insert));
			std::string message;
			try
			{
				LevelLoadedInfo(bad.string().c_str()).validate();
			}
			catch (const std::exception& error)
			{
				message = error.what();
			}
			require(message.find(candidate.phrase) != std::string::npos,
				std::string("validate() rejects the station ") + candidate.entry +
				" saying '" + candidate.phrase + "', got '" + message + "'");
		}
		std::error_code ignored;
		std::filesystem::remove(bad, ignored);
	}
#endif

#if COLOURWARS_DEV_TOOLS
	// ---------------------------------------------------------------- phase 3

	std::string read_text(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		require(stream.good(), "Could not read " + path.string());
		return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
	}

	void write_text(const std::filesystem::path& path, const std::string& text)
	{
		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		stream.close();
		require(!stream.fail(), "Could not write " + path.string());
	}

	std::string to_lf(std::string text)
	{
		std::string result;
		for (std::size_t i = 0; i < text.size(); ++i)
		{
			if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
			{
				continue;
			}
			result += text[i];
		}
		return result;
	}

	std::size_t count_of(const std::string& text, const std::string& what)
	{
		std::size_t count = 0;
		for (std::size_t at = text.find(what); at != std::string::npos; at = text.find(what, at + 1))
		{
			++count;
		}
		return count;
	}

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

	int count_of_lines(const DebugLog& log, const std::string& text)
	{
		int count = 0;
		for (const DebugLogLine& line : log.lines())
		{
			count += line.text.find(text) != std::string::npos ? 1 : 0;
		}
		return count;
	}

	std::string last_key(const std::string& path)
	{
		std::string key = path.substr(path.find_last_of('.') + 1);
		const std::size_t bracket = key.find('[');
		return bracket == std::string::npos ? key : key.substr(0, bracket);
	}

	void check_json_spans(const std::filesystem::path& game_directory)
	{
		for (const wchar_t* file : { L"tuning.json", L"presentation.json" })
		{
			const std::string text = read_text(game_directory / file);
			const JsonSpanDocument document(text, "content");
			require(document.leaves().size() > 50 && document.string_count() > 10,
				"A content file scans to its numbers, booleans and strings");
			for (const JsonLeaf& leaf : document.leaves())
			{
				require(document.replaced(leaf, document.token(leaf)) == text,
					"Re-splicing " + leaf.path + "'s own token reproduces the file byte for byte");
			}
			const JsonLeaf& first = document.leaves().front();
			const std::string changed = document.replaced(first, "12345");
			require(changed.compare(0, first.begin, text, 0, first.begin) == 0 &&
				changed.substr(first.begin + 5) == text.substr(first.end) &&
				changed.substr(first.begin, 5) == "12345", "A splice changes only that span");
		}

		const std::string synthetic = "{\r\n  \"a\": [1, [2, true]],\r\n  \"s\": \"q\\\"\xc3\xa9\",\r\n"
			"  \"b\": -1.5e3,\r\n  \"a\": 9,\r\n  \"o\": { \"x\": false }\r\n}\r\n";
		const JsonSpanDocument document(synthetic, "synthetic");
		std::vector<std::string> paths;
		std::string joined;
		for (const JsonLeaf& leaf : document.leaves())
		{
			paths.push_back(leaf.path + "=" + std::string(document.token(leaf)));
			joined += " " + paths.back();
		}
		require(paths == std::vector<std::string>{ "a[0]=1", "a[1][0]=2", "a[1][1]=true", "b=-1.5e3",
			"o.x=false" }, "Nested arrays get [i] paths, exponents and signs are one token, and a "
			"repeated key resolves to the first; got" + joined);

		const std::string spliced = document.replaced(*document.find("b"), "2");
		require(count_of(spliced, "\r\n") == count_of(synthetic, "\r\n") &&
			spliced.find("q\\\"\xc3\xa9") != std::string::npos,
			"CRLF and escapes survive a splice");
		for (const char* bad : { "{\"a\": 1,}", "{ // c\n \"a\": 1 }", "{\"a\": \"x }", "{\"a\": 1}\n2" })
		{
			std::string message;
			try
			{
				JsonSpanDocument(bad, "bad.json");
			}
			catch (const std::exception& error)
			{
				message = error.what();
			}
			require(message.find("bad.json: line ") != std::string::npos &&
				message.find(", column ") != std::string::npos,
				std::string("Malformed JSON fails with line and column: ") + bad + " gave '" + message + "'");
		}
	}

	void check_tune_scope(const std::filesystem::path& game_directory)
	{
		const TuneDocument tuning("tuning", read_text(game_directory / L"tuning.json"));
		const TuneDocument presentation("presentation", read_text(game_directory / L"presentation.json"));
		std::vector<std::string> paths = tuning.prefixed_paths();
		const std::vector<std::string> more = presentation.prefixed_paths();
		paths.insert(paths.end(), more.begin(), more.end());
		for (const std::string& pattern : tune_scope_patterns())
		{
			require(std::any_of(paths.begin(), paths.end(), [&pattern](const std::string& path)
				{ return tune_pattern_matches(pattern, path); }),
				"The scope row '" + pattern + "' matches a path the files have");
		}
		const std::pair<const char*, TuneScope> samples[] = {
			{ "tuning.player.gravity", TuneScope::live },
			{ "tuning.player.size.x", TuneScope::rebuild },
			{ "tuning.player.animations.running.frame_time", TuneScope::live },
			{ "tuning.match.duration", TuneScope::rebuild },
			{ "tuning.match.sounds.countdown.volume", TuneScope::live },
			{ "tuning.paint.thickness", TuneScope::rebuild },
			{ "tuning.weapons.sprayer.offset.x", TuneScope::live },
			{ "tuning.weapons.ammo_regen_rate", TuneScope::live },
			{ "tuning.projectiles.spray.size.x", TuneScope::new_objects },
			{ "tuning.projectiles.spray.gravity", TuneScope::live },
			{ "presentation.movement_feedback.minimum_strength", TuneScope::live },
			{ "presentation.camera.reference_height", TuneScope::live },
			{ "presentation.match.countdown_center_y", TuneScope::rebuild },
			{ "presentation.main_menu.title_position.x", TuneScope::next_screen },
		};
		for (const auto& [path, scope] : samples)
		{
			require(std::find(paths.begin(), paths.end(), path) != paths.end(),
				std::string("The files have ") + path);
			require(tune_scope_of(path) == scope, std::string("Scope of ") + path + " is " +
				tune_scope_name(scope) + ", got " + tune_scope_name(tune_scope_of(path)));
		}
		require(tune_scope_of("tuning.never.seen") == TuneScope::rebuild, "Unmatched paths rebuild");

		require(tune_step(1800.0, false) == 100.0 && std::fabs(tune_step(0.15, false) - 0.01) < 1e-12 &&
			tune_step(0.0, false) == 0.1, "Decimal steps are one unit of the digit below the leading one");
		require(tune_step(4.0, true) == 1.0 && tune_step(0.0, true) == 1.0 && tune_step(12.0, true) == 1.0 &&
			tune_step(250.0, true) == 10.0, "Integer tokens step by whole numbers, at least 1");
		require(tune_format(250.0, "240.0") == "250.0" && tune_format(5.0, "4") == "5" &&
			tune_format(0.16, "0.15") == "0.16" && tune_format(4.5, "4") == "4.5" &&
			tune_format(-650.0, "-600.0") == "-650.0", "Formatting keeps the token's style");

		const std::vector<std::string> tree = { "a.b.c", "a.b.d", "a.e", "f[0].x", "f[1].x", "g[2]" };
		require(tune_children(tree, "") == std::vector<std::string>{ "a.", "f[", "g[" } &&
			tune_children(tree, "a.") == std::vector<std::string>{ "a.b.", "a.e" } &&
			tune_children(tree, "f[") == std::vector<std::string>{ "f[0].", "f[1]." } &&
			tune_children(tree, "g[") == std::vector<std::string>{ "g[2]" },
			"tune_children walks the tree one segment at a time");
	}

	// Every number and boolean is read by the game: replacing one with a
	// string must make the reader throw and name its key. Otherwise the file
	// has a key nothing reads and `tune` on it would do nothing.
	int check_tune_consumed(const std::filesystem::path& game_directory)
	{
		const std::filesystem::path directory = std::filesystem::temp_directory_path() /
			(L"colourwars_consumed_" + std::to_wstring(_getpid()));
		std::filesystem::create_directories(directory);
		int checked = 0;
		for (const char* name : { "tuning", "presentation" })
		{
			const std::string text = read_text(game_directory / (std::string(name) + ".json"));
			const JsonSpanDocument document(text, name);
			const std::filesystem::path candidate = directory / (std::string(name) + ".json");
			for (const JsonLeaf& leaf : document.leaves())
			{
				bool comment = false;
				for (std::size_t at = 0; at < leaf.path.size(); at = leaf.path.find('.', at) + 1)
				{
					comment = comment || leaf.path[at] == '_';
					if (leaf.path.find('.', at) == std::string::npos) { break; }
				}
				if (comment)
				{
					continue;
				}
				write_text(candidate, document.replaced(leaf, "\"__unread__\""));
				std::string message;
				try
				{
					if (std::string(name) == "tuning")
					{
						GameTuning loaded(candidate.string().c_str());
					}
					else
					{
						Presentation loaded(candidate.string().c_str());
					}
				}
				catch (const std::exception& error)
				{
					message = error.what();
				}
				require(!message.empty(), std::string(name) + "." + leaf.path +
					" is in the file but the game never reads it");
				require(message.find(last_key(leaf.path)) != std::string::npos,
					std::string(name) + "." + leaf.path + ": the reader's message does not name '" +
					last_key(leaf.path) + "': " + message);
				++checked;
			}
		}
		std::error_code ignored;
		std::filesystem::remove_all(directory, ignored);
		return checked;
	}

	void check_tune_commands(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"tune");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const DebugLog& log = impl.log();

		// The D2 guard: string buffers the game points at stay put.
		const char* const bank = tuning.player().sound_bank.data();
		const char* const meter_font = presentation.hud().meter_label_font.data();
		const WeaponTuning* const sprayer = &tuning.weapon(WeaponType::sprayer);

		// Every value below is relative to the file's, so a balance pass on
		// tuning.json never fails this check.
		const std::string gravity = impl.tune_token("tuning.player.gravity");
		require(!gravity.empty(), "The scratch file has a player gravity");
		const double base = std::stod(gravity);
		const auto token = [&gravity](double value) { return tune_format(value, gravity); };
		const std::string up = token(base + 100.0);
		require(impl.execute("tune tuning.player.gravity " + up).ok &&
			tuning.player().gravity == static_cast<float>(base + 100.0), "tune sets a live number");
		require(log_contains(log, "tune: tuning.player.gravity " + gravity + " -> " + up + " (live)",
			DebugSeverity::info), "tune reports old, new and the scope");
		require(impl.execute("tune tuning.player.gravity").ok &&
			log_contains(log, "tuning.player.gravity = " + up + "  (file: " + gravity + ")  [live]",
				DebugSeverity::info), "tune without a value prints the value, the file's and the scope");

		require(!impl.execute("tune tuning.player.jump_buffer_time 2").ok &&
			log_contains(log, "outside [0, 1]", DebugSeverity::error) &&
			tuning.player().jump_buffer_time != 2.0f, "The reader's own range rule rejects a value");
		require(!log_contains(log, "ColourWars-tune-", DebugSeverity::error),
			"A rejection names the content file, not the temporary candidate");
		require(!impl.execute("tune presentation.movement_feedback.minimum_strength 1.5").ok &&
			log_contains(log, "strength must be at most one", DebugSeverity::error),
			"A cross-field rule rejects a value");
		require(!impl.execute("tune tuning.player.sound_bank x").ok &&
			!impl.execute("tune tuning.nowhere 1").ok && !impl.execute("tune player.gravity 1").ok,
			"Strings, unknown paths and unprefixed paths are refused");

		const double stepped = base + 100.0 + tune_step(base + 100.0, tune_integer_token(up));
		require(impl.execute("tune tuning.player.gravity next").ok &&
			tuning.player().gravity == static_cast<float>(stepped) &&
			impl.tune_token("tuning.player.gravity") == token(stepped), "next steps by tune_step");
		require(impl.execute("tune tuning.player.gravity -=150").ok &&
			tuning.player().gravity == static_cast<float>(stepped - 150.0), "-= subtracts");

		// Booleans, and the F9 switch beside them.
		require(impl.execute("tune presentation.movement_feedback.enabled off").ok &&
			!presentation.movement_feedback().enabled, "A boolean tunes with on/off words");
		require(impl.execute("feedback on").ok && presentation.movement_feedback().enabled,
			"F9 still toggles the live switch");
		require(impl.execute("tune presentation.movement_feedback.full_impact_speed next").ok &&
			presentation.movement_feedback().enabled,
			"Tuning another presentation value keeps the live F9 state");
		impl.execute("tune.diff");
		require(log_contains(log, "live toggle (F9) is on, not a file edit", DebugSeverity::info),
			"tune.diff says when F9 disagrees with the file");
		require(log_contains(log, "  tuning.player.gravity  " + gravity + " -> " + token(stepped - 150.0) +
			"  [live]", DebugSeverity::info) &&
			log_contains(log, "  presentation.movement_feedback.enabled  true -> false  [live]",
				DebugSeverity::info) && log_contains(log, "tune.diff: 4 line(s)", DebugSeverity::info),
			"tune.diff lists exactly the edits");
		require(tuning.player().sound_bank.data() == bank &&
			presentation.hud().meter_label_font.data() == meter_font &&
			&tuning.weapon(WeaponType::sprayer) == sprayer,
			"Copy-assigning a candidate keeps every string buffer and record address");

		require(impl.execute("tune.reset presentation").ok && presentation.movement_feedback().enabled &&
			impl.tune_document("presentation")->edits().empty(),
			"tune.reset of a file reverts its edits, the toggle edit included");
		require(impl.execute("tune.reset tuning.player.gravity").ok &&
			tuning.player().gravity == static_cast<float>(base) &&
			impl.tune_document("tuning")->edits().empty(), "tune.reset of a path reverts it");

		// A rebuild-scoped value with restart, in a match.
		FakeMatch match(test.director);
		match.settings = planned(1);
		test.director.attach_match(match);
		const float thickness = tuning.paint().thickness;
		const std::string thicker = tune_format(static_cast<double>(thickness) + 1.0,
			impl.tune_token("tuning.paint.thickness"));
		const DebugStatus restart = impl.execute("tune tuning.paint.thickness " + thicker + " restart");
		require(restart.ok && restart.close_console && impl.request().restart &&
			tuning.paint().thickness == thickness + 1.0f, "restart after a rebuild-scoped tune queues a rebuild");
		require(test.director.before_match_update(match, 1.0f / 60.0f) && match.rebuilds == 1,
			"and the match rebuilds");
		test.director.detach_match(match);
		require(impl.execute("tune.reset").ok && tuning.paint().thickness == thickness, "tune.reset all");

		// tune.save, CRLF tuning: exactly that token changes.
		const std::string tuning_before = read_text(scratch.source / L"tuning.json");
		require(impl.execute("tune tuning.player.gravity " + up).ok, "tune before save");
		require(impl.execute("tune.save tuning").ok, "tune.save writes the source file");
		// The player's gravity is the file's first "gravity" key.
		std::string expected = tuning_before;
		const std::string old_key = "\"gravity\": " + gravity;
		const std::size_t at = expected.find(old_key);
		require(at != std::string::npos && at == expected.find("\"gravity\": "),
			"The source's first gravity is the player's " + gravity);
		expected.replace(at, old_key.size(), "\"gravity\": " + up);
		const std::string tuning_after = read_text(scratch.source / L"tuning.json");
		require(tuning_after == expected, "tune.save changes only the edited token (CRLF file)");
		require(read_text(scratch.staged / L"tuning.json") == tuning_after,
			"tune.save refreshes the staged copy");
		require(!std::filesystem::exists(scratch.source / L"tuning.json.tmp") &&
			!std::filesystem::exists(scratch.staged / L"tuning.json.tmp"),
			"tune.save leaves no temporary beside either copy");
		require(GameTuning((scratch.source / L"tuning.json").string().c_str()).player().gravity ==
			static_cast<float>(base + 100.0), "The saved file loads with the new value");
		require(impl.tune_document("tuning")->edits().empty() &&
			impl.tune_document("tuning")->baseline().text() == tuning_after,
			"After a save the baseline is the file and tune.diff is empty");

		// LF presentation.
		const std::string presentation_before = read_text(scratch.source / L"presentation.json");
		const std::string strength_was = impl.tune_token("presentation.movement_feedback.minimum_strength");
		require(impl.execute("tune presentation.movement_feedback.minimum_strength prev").ok &&
			impl.execute("tune.save").ok, "tune.save all");
		const std::string strength_now = impl.tune_token("presentation.movement_feedback.minimum_strength");
		expected = presentation_before;
		const std::size_t strength = expected.find("\"minimum_strength\": " + strength_was);
		require(strength != std::string::npos && strength_now != strength_was,
			"The source has minimum_strength " + strength_was);
		expected.replace(strength, std::string("\"minimum_strength\": " + strength_was).size(),
			"\"minimum_strength\": " + strength_now);
		require(read_text(scratch.source / L"presentation.json") == expected &&
			read_text(scratch.source / L"presentation.json").find('\r') == std::string::npos,
			"tune.save changes only the edited token (LF file)");

		// A token changed on disk since it was loaded is a conflict.
		const std::string ours = token(base + 200.0);
		const std::string theirs = token(base + 50.0);
		require(impl.execute("tune tuning.player.gravity " + ours).ok, "tune before a conflict");
		std::string external = tuning_after;
		external.replace(external.find("\"gravity\": " + up), std::string("\"gravity\": " + up).size(),
			"\"gravity\": " + theirs);
		write_text(scratch.source / L"tuning.json", external);
		require(!impl.execute("tune.save tuning").ok &&
			log_contains(log, "conflict: tuning.player.gravity: changed on disk to " + theirs,
				DebugSeverity::error) && read_text(scratch.source / L"tuning.json") == external,
			"A conflict names the path and writes nothing");
		require(impl.execute("tune.save tuning force").ok &&
			read_text(scratch.source / L"tuning.json").find("\"gravity\": " + ours) != std::string::npos,
			"force overwrites the value changed on disk");
		require(impl.execute("tune.save").ok && log_contains(log, "tune.save: no edits to save",
			DebugSeverity::info), "Nothing to save says so");

		// No source tree: tune.save says what it needs.
		impl.set_content_directories(scratch.root / L"missing", scratch.staged);
		require(impl.execute("tune tuning.player.gravity " + token(base + 300.0)).ok &&
			!impl.execute("tune.save").ok &&
			log_contains(log, "tune.save needs a Debug build made from a checkout", DebugSeverity::error),
			"Without a source tree tune.save fails and says why");
		impl.execute("tune.reset");
		require(!std::filesystem::exists(std::filesystem::temp_directory_path() /
			(L"ColourWars-tune-" + std::to_wstring(_getpid()))),
			"Validation leaves no candidate directory in %TEMP%");
	}

	void check_tune_completion_and_menu(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"menu");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const DebugRegistry& registry = impl.registry();

		require(registry.complete("tune tuning.pl").line == "tune tuning.player.",
			"A path completes one segment, with no space after it");
		require(registry.complete("tune tuning.player.jump_b").line ==
			"tune tuning.player.jump_buffer_time ", "A leaf completes with a space");
		const std::vector<std::string> groups = registry.complete("tune presentation.").candidates;
		require(std::find(groups.begin(), groups.end(), "presentation.hud.") != groups.end() &&
			std::find(groups.begin(), groups.end(), "presentation.camera.") != groups.end(),
			"presentation. lists its groups");
		require(registry.complete("tune tuning.player.gravity ").candidates ==
			std::vector<std::string>{ "next", "prev" } &&
			registry.complete("tune presentation.movement_feedback.enabled ").candidates ==
			std::vector<std::string>{ "off", "on", "toggle" }, "The value completes by kind");

		DebugMenuModel model(impl);
		const std::vector<std::string> roots = labels(model.rows());
		require(std::find(roots.begin(), roots.end(), "tuning") != roots.end() &&
			std::find(roots.begin(), roots.end(), "replay") != roots.end(),
			"The root lists the tuning and replay categories");
		model.confirm(row_index(model.rows(), "tuning"));
		require(labels(model.rows()) == std::vector<std::string>{ "tuning", "presentation", "tune.diff",
			"tune.reset", "tune.save" }, "Tuning opens on the two files and the save rows");
		model.confirm(row_index(model.rows(), "tuning"));
		require(model.title() == "DEBUG / tuning", "A file node's title");
		model.confirm(row_index(model.rows(), "player"));
		require(model.title() == "DEBUG / tuning / player", "The node's title is its path");
		std::vector<DebugMenuRow> rows = model.rows();
		require(rows.size() == DebugMenuModel::max_rows && rows.back().kind == DebugMenuRow::Kind::more &&
			rows.back().label.rfind("more (1/", 0) == 0, "A long node is paged with a more row");
		const std::size_t gravity = row_index(rows, "gravity");
		const std::string gravity_token = impl.tune_token("tuning.player.gravity");
		const float gravity_value = tuning.player().gravity;
		require(rows[gravity].kind == DebugMenuRow::Kind::variable && rows[gravity].value == gravity_token,
			"A leaf shows its token");
		require(rows[row_index(rows, "size")].kind == DebugMenuRow::Kind::category,
			"An inner node is a category row");
		model.step(gravity, 1);
		require(tuning.player().gravity == static_cast<float>(gravity_value +
			tune_step(gravity_value, tune_integer_token(gravity_token))) &&
			log_contains(impl.log(), "[menu] tune tuning.player.gravity next", DebugSeverity::echo),
			"Right on a leaf runs tune <path> next, echoed");
		model.step(gravity, -1);
		require(tuning.player().gravity == gravity_value, "Left runs prev");
		const std::string first_page = labels(model.rows()).front();
		model.confirm(model.rows().size() - 1);
		require(labels(model.rows()).front() != first_page &&
			model.rows().back().label.rfind("more (2/", 0) == 0, "more advances the page");
		model.back();
		require(model.title() == "DEBUG / tuning" &&
			model.rows()[model.focus_hint()].label == "player", "Back pops one segment onto the node it left");
		model.confirm(row_index(model.rows(), "player"));
		model.confirm(row_index(model.rows(), "size"));
		require(model.title() == "DEBUG / player / size" &&
			model.rows()[row_index(model.rows(), "y")].value.find("*rebuild") != std::string::npos,
			"A value that is not live is marked with its scope");
		model.back();
		model.back();
		model.back();
		require(model.title() == "DEBUG / tuning", "Back reaches the category");
		model.back();
		require(model.title() == "DEBUG", "and then the root");

		// The presentation boolean steps by toggle.
		model.confirm(row_index(model.rows(), "tuning"));
		model.confirm(row_index(model.rows(), "presentation"));
		model.confirm(row_index(model.rows(), "movement_feedback"));
		const bool enabled = presentation.movement_feedback().enabled;
		model.step(row_index(model.rows(), "enabled"), -1);
		require(presentation.movement_feedback().enabled != enabled,
			"Left or Right on a boolean toggles it");
		impl.execute("tune.reset");
	}

	// A view sequence that moves, lands, stalls and meets an opponent.
	BotSeatView scripted_view(int step)
	{
		BotSeatView view;
		view.step = step;
		view.self.center = { 1000.0f + 3.0f * static_cast<float>(step % 200), 800.0f };
		view.self.velocity = { step % 70 < 25 ? 0.0f : 300.0f, step % 40 < 20 ? 0.0f : -200.0f };
		view.self.team = PlayerTeam::a;
		view.self.on_ground = step % 40 < 20;
		view.self.ammo = static_cast<float>(100 - step % 100) / 100.0f;
		BotSeatView::Seat opponent;
		opponent.center = { 1200.0f - static_cast<float>(step % 300), 600.0f + static_cast<float>(step % 50) };
		opponent.team = PlayerTeam::b;
		opponent.alive = step % 150 < 120;
		view.others.push_back(opponent);
		return view;
	}

	std::vector<std::string> run_bot(const std::string& behaviour, std::uint64_t seed, int steps)
	{
		std::unique_ptr<Bot> bot = Bot::make(behaviour, seed, 2);
		require(bot != nullptr && bot->name() == behaviour, "Bot::make builds " + behaviour);
		std::vector<std::string> outputs;
		bool held = false;
		for (int step = 0; step < steps; ++step)
		{
			const PlayerInputData input = bot->think(scripted_view(step));
			require(input.connected && !input.toggle_pause_menu && !input.toggle_debug,
				behaviour + ": a bot is connected and never opens a menu or the F1 overlay");
			require(!input.jump_pressed || (input.jump_held && !held),
				behaviour + ": jump_pressed only on the step a hold begins");
			held = input.jump_held;
			if (input.shoot_direction_requested)
			{
				require(std::fabs(input.shoot_direction.length() - 1.0f) < 1e-4f,
					behaviour + ": the aim is a unit vector");
			}
			outputs.push_back(replay_codec::encode(input));
		}
		return outputs;
	}

	void check_bots(GameTuning& tuning, Presentation& presentation)
	{
		for (const std::string& behaviour : { "walk", "jump", "chase", "paint" })
		{
			const std::vector<std::string> first = run_bot(behaviour, 7, 600);
			require(run_bot(behaviour, 7, 600) == first, behaviour + " is deterministic for a seed");
			if (behaviour != "chase")
			{
				require(run_bot(behaviour, 8, 600) != first, behaviour + " differs with another seed");
			}
		}
		require(Bot::make("idle", 1, 0) == nullptr && Bot::is_behaviour("idle") &&
			!Bot::is_behaviour("dance"), "idle is a word for neutral input, not a class");
		const std::vector<std::string> paint = run_bot("paint", 3, 600);
		require(std::any_of(paint.begin(), paint.end(), [](const std::string& text)
			{ PlayerInputData input; return replay_codec::decode(text, input) && input.primary_shoot; }) &&
			std::any_of(paint.begin(), paint.end(), [](const std::string& text)
			{ PlayerInputData input; return replay_codec::decode(text, input) && input.jump_pressed; }),
			"A paint bot shoots and jumps");

		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		DebugDirector& director = test.director;
		require(impl.execute("bot all paint").ok && impl.execute("bot 1 chase").ok, "bot assigns");
		FakeMatch match(director);
		match.settings = planned(4);
		director.attach_match(match);
		director.after_level_build(match);
		require(impl.status_strip().find("BOTS 4") != std::string::npos,
			"bot all drives the dummies and bot 1 the keyboard seat: " + impl.status_strip());
		require(director.drives_slot(input_slots::keyboard_and_mouse) && !director.drives_slot(0),
			"A bot on the keyboard seat drives its slot");
		std::vector<PlayerInputData> inputs(input_slots::count);
		director.update_match(match, inputs, 1.0f / 60.0f);
		require(match.last_filter != nullptr, "With bots, the update gets a seat filter");
		require(impl.execute("bot 1 off").ok && impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"bot all paint drives only the three idle dummies: " + impl.status_strip());
		require(!director.drives_slot(input_slots::keyboard_and_mouse), "bot 1 off hands the seat back");
		require(impl.execute("bot off").ok && impl.status_strip().find("BOTS") == std::string::npos,
			"bot off clears every bot");
		director.update_match(match, inputs, 1.0f / 60.0f);
		require(match.last_filter == nullptr, "Without bots the update gets no seat filter");
		require(!impl.execute("bot 1 dance").ok && !impl.execute("bot 9 walk").ok && !impl.execute("bot 2").ok,
			"bot checks its seat and behaviour");

		// players N <bot>: the dummies of the rebuilt match get that bot.
		require(impl.execute("players 4 paint").ok && impl.request().players == 4,
			"players 4 paint queues four players");
		require(director.before_match_update(match, 1.0f / 60.0f) && match.rebuilds == 1 &&
			impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"and the rebuild puts paint bots on the three dummies");
		impl.execute("status");
		require(log_contains(impl.log(), " idle team b sprayer bot paint", DebugSeverity::info),
			"status names each seat's bot");
		director.detach_match(match);
		impl.execute("bot off");

		// A bot on a pad seat: that pad's absence never pauses the match.
		MatchRequest roster;
		roster.stage = "x";
		roster.players = 2;
		unsigned int seed = 1;
		const MatchPlan plan = MatchPlanner(&test_stages()).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{ true, false, false, false }, [&seed] { return seed++; });
		require(plan.ok && plan.settings.player_settings()[0].num == 0, "Seat 1 is pad 1");
		FakeMatch pad_match(director);
		pad_match.settings = plan.settings;
		director.attach_match(pad_match);
		labrador::GamepadReader reader;
		labrador::Gamepads pads(&reader);
		require(disconnected_player_slot(pad_match.settings, pads, director) == 0,
			"An unplugged pad seat pauses the match");
		impl.execute("bot 1 walk");
		require(director.drives_slot(0) && disconnected_player_slot(pad_match.settings, pads, director) == -1 &&
			!pad_seat_missing(0, pads, director), "A bot on that seat keeps it from pausing");
		impl.execute("bot off");
		director.detach_match(pad_match);
	}

	void check_seeded_planner(GameTuning& tuning, Presentation& presentation)
	{
		const auto weapons_for = [&](const char* seed)
			{
				TestDirector test(tuning, presentation);
				test.impl.execute(std::string("seed ") + seed);
				test.impl.execute("stage x");
				test.impl.execute("players 4");
				test.impl.execute("weapon all random");
				FakePage page;
				page.start_result = true;
				require(test.director.before_page_update(page, 1.0f / 60.0f) && page.starts == 1,
					"A seeded stage starts from the menus");
				std::string weapons;
				for (const MenuPlayerSettings& seat : page.started.player_settings())
				{
					weapons += MatchPlanner::weapon_name(seat.weapon) + std::string(" ");
				}
				return weapons;
			};
		const std::string first = weapons_for("5");
		require(weapons_for("5") == first, "The same seed gives the same random weapons");
		bool differs = false;
		for (const char* other : { "6", "7", "8", "9" })
		{
			differs = differs || weapons_for(other) != first;
		}
		require(differs, "Other seeds give other weapons");

		// Not only the first plan after `seed`: every plan under it.
		TestDirector test(tuning, presentation);
		test.impl.execute("seed 5");
		const auto start = [&]
			{
				test.impl.execute("stage x");
				test.impl.execute("players 4");
				test.impl.execute("weapon all random");
				FakePage page;
				page.start_result = true;
				require(test.director.before_page_update(page, 1.0f / 60.0f) && page.starts == 1,
					"A seeded stage starts from the menus");
				std::string weapons;
				for (const MenuPlayerSettings& seat : page.started.player_settings())
				{
					weapons += MatchPlanner::weapon_name(seat.weapon) + std::string(" ");
				}
				return weapons;
			};
		require(start() == first && start() == first && start() == first,
			"Every plan under one seed draws the same random weapons");
	}

	bool same_inputs(const std::vector<std::vector<PlayerInputData>>& a,
		const std::vector<std::vector<PlayerInputData>>& b)
	{
		if (a.size() != b.size()) { return false; }
		for (std::size_t seat = 0; seat < a.size(); ++seat)
		{
			if (a[seat].size() != b[seat].size()) { return false; }
			for (std::size_t i = 0; i < a[seat].size(); ++i)
			{
				if (!replay_codec::same(a[seat][i], b[seat][i])) { return false; }
			}
		}
		return true;
	}

	void check_replay_codec()
	{
		PlayerInputData input;
		input.x_movement = -0.0f;
		input.shoot_direction = { 1e-40f, -1.0f };
		input.shoot_angle = 3.14159265f;
		input.left_analog_stick = { 0.25f, -0.75f };
		input.right_analog_stick = { std::nextafter(0.5f, 1.0f), 0.0f };
		input.jump_pressed = input.jump_held = input.primary_shoot = input.connected = true;
		input.drop_through = true;
		PlayerInputData decoded;
		require(replay_codec::decode(replay_codec::encode(input), decoded) &&
			std::bit_cast<std::uint32_t>(decoded.x_movement) == std::bit_cast<std::uint32_t>(-0.0f) &&
			decoded.shoot_direction.x == 1e-40f && decoded.right_analog_stick.x == input.right_analog_stick.x &&
			decoded.jump_pressed && decoded.drop_through && !decoded.toggle_debug &&
			replay_codec::same(decoded, input), "An input encodes and decodes bit-exactly (-0.0, denormals)");
		require(!replay_codec::decode("zz|0|0", decoded) && !replay_codec::decode("80|0|0|0|0|0|0|0|q", decoded),
			"Malformed inputs do not decode");

		std::vector<PlayerInputData> stream(5, input);
		stream.push_back(PlayerInputData());
		stream.insert(stream.end(), 3, input);
		const std::vector<std::string> entries = replay_codec::run_length(stream);
		require(entries.size() == 3 && entries[0].rfind("5*", 0) == 0 && entries[2].rfind("3*", 0) == 0,
			"Runs compress");
		const std::vector<PlayerInputData> expanded = replay_codec::expand(entries);
		require(same_inputs({ expanded }, { stream }), "Runs expand back");
		require(replay_name("my run!") == "my_run_" && replay_name("").rfind("replay-", 0) == 0,
			"Replay names are safe file stems");

		const std::filesystem::path directory = std::filesystem::temp_directory_path() /
			(L"colourwars_replay_codec_" + std::to_wstring(_getpid()));
		const std::filesystem::path path = directory / L"round.json";
		ReplayFile file;
		file.build = "test \"build\"";
		file.dt_bits = std::bit_cast<std::uint32_t>(1.0f / 60.0f);
		file.seed = 4000000000u;
		file.tuning_hash = 0x0123456789abcdefull;
		file.presentation_hash = 42;
		file.tune_diff = { "tuning.player.gravity 2000.0 -> 2100.0" };
		file.god = true;
		file.settings = planned(3);
		file.settings.set_workshop_visual_mode(PlayerVisualMode::authored);
		file.settings.set_player_weapon(1, WeaponType::roller);
		file.updates = 120;
		file.suspends = { 0, 61 };
		file.inputs = { stream, std::vector<PlayerInputData>(9), expanded };
		file.checkpoints = { { 60, 0xfeedfacecafebeefull }, { 120, 7 } };
		file.final_print.updates = 120;
		file.final_print.hash = 99;
		file.final_print.team_a = std::bit_cast<std::uint32_t>(12.5f);
		file.final_print.seats.push_back({ 1, 2, 3, 4, 5, 6, 7, 2, 1 });
		file.write(path);
		const ReplayFile read = ReplayFile::read(path);
		require(read.build == file.build && read.dt_bits == file.dt_bits && read.seed == file.seed &&
			read.tuning_hash == file.tuning_hash && read.presentation_hash == 42 &&
			read.tune_diff == file.tune_diff && read.god && !read.infinite_ammo &&
			read.updates == 120 && read.suspends == file.suspends &&
			same_inputs(read.inputs, file.inputs) && read.checkpoints.size() == 2 &&
			read.checkpoints[0].hash == 0xfeedfacecafebeefull &&
			MatchFingerprint::difference(file.final_print, read.final_print).empty() &&
			read.final_print.hash == 99, "A replay file round-trips");
		const std::vector<MenuPlayerSettings> seats = read.settings.player_settings();
		require(seats.size() == 3 && seats[0].num == input_slots::keyboard_and_mouse && seats[1].num == -1 &&
			seats[1].weapon == WeaponType::roller && seats[1].team == PlayerTeam::b &&
			read.settings.stage() == "x" && read.settings.workshop_visual_mode() == PlayerVisualMode::authored &&
			read.settings.screen_layout() == labrador::ScreenLayout::three_player,
			"The recorded settings rebuild verbatim");

		std::string text = read_text(path);
		text.replace(text.find("\"version\": 1"), 12, "\"version\": 2");
		write_text(path, text);
		std::string message;
		try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
		require(message.find("format version 2; this build reads 1") != std::string::npos,
			"Another format version is refused: " + message);
		write_text(path, "{ \"version\": 1, ");
		message.clear();
		try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
		require(message.find("not valid JSON") != std::string::npos, "A broken file is refused: " + message);
		for (const char* layout : { "\"layout\": 7,", "\"layout\": 0," })
		{
			file.write(path);
			text = read_text(path);
			text.replace(text.find("\"layout\": 2,"), 12, layout);
			write_text(path, text);
			message.clear();
			try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
			require(message.find("is not the 3-player layout") != std::string::npos,
				std::string("A replay with ") + layout + " for three seats is refused: " + message);
		}
		std::error_code ignored;
		std::filesystem::remove_all(directory, ignored);
	}

	// ------------------------------------------- a real Level on the CPU

	// What LevelBuilder needs, with no window or device: sprite sheets and
	// fonts whose atlases are null handles, a silent bank, the levels, and
	// the shell services by value.
	struct CpuWorld
	{
		labrador::RenderResources render;
		labrador::AudioResources audio;
		PlaySettings preferences;
		labrador::Registry<LevelLoadedInfo> levels{ "Level" };
		StageList stages;
		labrador::ResolutionManager resolution;
		labrador::ViewportManager viewports{ &resolution };
		labrador::Mouse mouse;
		labrador::Partitioner partitioner;
		std::unique_ptr<LevelBuilder> builder;

		CpuWorld(const std::filesystem::path& content, const GameTuning& tuning,
			const Presentation& presentation)
		{
			for (const char* sheet : { "sprite_sheet_1", "menu_paint", "weapon_paint", "workshop_kit" })
			{
				this->render.add_sprite_sheet(sheet, labrador::read_sprite_sheet(
					(content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
					labrador::TextureHandle{}));
			}
			for (const char* font : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
				"gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
				"gill_sans_mt_bold_144", "courier_new_bold_16" })
			{
				const labrador::SpriteFontFile file = labrador::read_sprite_font_file(
					(content / "fonts" / (std::string(font) + ".spritefont")).string());
				auto loaded = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs,
					file.line_spacing);
				if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
				this->render.add_font(font, std::move(loaded));
			}
			this->audio.add_sound_bank("sound_bank_1", labrador::SoundBank::silent());
			for (const char* level : { "movement_course", "close_quarters" })
			{
				auto info = std::make_unique<LevelLoadedInfo>(
					(content / "levels" / (std::string(level) + ".json")).string().c_str());
				info->validate();
				this->levels.add(level, std::move(info));
			}
			this->stages = { { "movement_course", "Movement Course", "icon", false },
				{ "close_quarters", "Close Quarters", "icon", true } };
			this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render, &this->audio,
				&this->preferences, &this->levels, &tuning, &presentation, &this->resolution, nullptr,
				&this->partitioner, &this->mouse);
		}
	};

	// GameLevel's debug face over a real Level: one build path through
	// before_level_build and after_level_build, as GameLevel has.
	class CpuMatch final : public DebugMatchTarget
	{
	public:
		CpuMatch(CpuWorld& world, DebugDirector& director) : world_(world), director_(director) {}

		MenuLevelSettings settings;
		std::unique_ptr<Level> level;
		int builds = 0;

		void build()
		{
			this->level.reset();
			this->director_.before_level_build(*this);
			this->world_.viewports.set_layout(this->settings.screen_layout());
			this->level = this->world_.builder->build_level(this->settings, TeamColourPair::blue_orange);
			++this->builds;
			this->director_.after_level_build(*this);
		}

		// What GameLevel::on_suspend does when a state covers the match.
		void suspend()
		{
			this->level->suspend();
			this->director_.match_suspended(*this);
		}

		const MenuLevelSettings& debug_settings() const override { return this->settings; }
		void debug_rebuild(const MenuLevelSettings& plan, bool) override
		{
			this->settings = plan;
			this->build();
		}
		void debug_adopt(const MenuLevelSettings& plan, bool) override { this->settings = plan; }
		void debug_leave() override {}
		void debug_update_level(const std::vector<PlayerInputData>& inputs, float dt,
			SeatInputFilter* seats) override
		{
			this->level->update(inputs, dt, seats);
		}
		bool debug_level_finished() const override { return this->level->state() == LevelState::finished; }
		const Level* debug_level() const override { return this->level.get(); }
		// GameLevel's, uncovered and in focus: only the debug clock pauses it,
		// which pauses the voices and leaves the players alone.
		void debug_sync_audio() override
		{
			if (this->director_.match_audio_paused())
			{
				this->level->pause_audio();
			}
			else
			{
				this->level->resume();
			}
		}
		std::vector<std::string> debug_status() const override { return {}; }
		void debug_set_overlays(const LevelOverlays& overlays) override { this->level->set_overlays(overlays); }
		void debug_set_player_text(bool showing) override { this->level->set_players_showing_debug(showing); }
		void debug_set_cheats(bool immune, bool infinite) override
		{
			this->god = immune;
			this->level->set_player_cheats(immune, infinite);
		}
		bool god = false;
		std::vector<LevelStation> debug_stations() const override
		{
			return this->world_.levels.get(this->settings.stage())->stations();
		}
		bool debug_place(int seat, const mattmath::Vector2F& top_left) override
		{
			return this->level->place_player(seat, top_left);
		}
		bool debug_respawn(int seat) override { return this->level->respawn_player(seat); }
		MatchPaint debug_paint() const override
		{
			const LevelEndInfo info = this->level->level_end_info();
			MatchPaint paint;
			paint.team_a = info.team_a_area;
			paint.team_b = info.team_b_area;
			paint.total = info.total_area;
			return paint;
		}

	private:
		CpuWorld& world_;
		DebugDirector& director_;
	};

	// The keyboard seat's own scripted input (the bots drive the rest).
	std::vector<PlayerInputData> keyboard_step(int step)
	{
		std::vector<PlayerInputData> inputs(input_slots::count);
		PlayerInputData& keys = inputs[static_cast<std::size_t>(input_slots::keyboard_and_mouse)];
		keys.connected = true;
		keys.x_movement = (step / 90) % 3 == 0 ? 1.0f : (step / 90) % 3 == 1 ? -1.0f : 0.0f;
		keys.jump_held = step % 75 < 12;
		keys.jump_pressed = step % 75 == 0;
		keys.primary_shoot = step % 40 < 25;
		return inputs;
	}

	// Steps the match until `done`, the way GameLevel does: the director's
	// hooks, then its clock's updates. Returns the steps taken.
	template <typename Done>
	int drive(DebugDirector& director, CpuMatch& match, float dt, int limit, Done done,
		int suspend_at = -1)
	{
		DebugDirectorImpl& impl = *director.impl();
		int step = 0;
		bool suspended = false;
		for (; step < limit && !done(); ++step)
		{
			if (director.before_match_update(match, dt))
			{
				continue;
			}
			if (!suspended && suspend_at >= 0 && impl.level_updates() == suspend_at)
			{
				match.suspend();
				suspended = true;
			}
			director.update_match(match, keyboard_step(impl.level_updates()), dt);
		}
		return step;
	}

	std::string check_replay_cpu_match(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"replay");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		CpuWorld world(game_directory, tuning, presentation);
		GameData data;
		data.set_stages(&world.stages);
		data.set_level_infos(&world.levels);
		DebugDirector director(&tuning, &presentation);
		director.attach(nullptr, &data);
		DebugDirectorImpl& impl = *director.impl();
		impl.log().set_mirror_to_debugger(false);
		impl.set_content_directories(scratch.source, scratch.staged);
		const DebugLog& log = impl.log();
		const float dt = 1.0f / 60.0f;
		const int length = 600;

		// +seed 7 +stage movement_course +players 4 +bot all paint, built directly.
		require(impl.execute("seed 7").ok && impl.execute("bot all paint").ok, "seed and bot in the menus");
		MatchRequest roster;
		roster.stage = "movement_course";
		roster.players = 4;
		roster.visual = PlayerVisualMode::authored;
		const MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [] { return 0u; });
		require(plan.ok, "The CPU roster resolves");
		CpuMatch match(world, director);
		match.settings = plan.settings;
		director.attach_match(match);
		match.build();
		require(impl.status_strip().find("BOTS 3x paint") != std::string::npos, "Three paint bots");

		// Record 600 updates, with a suspend (a console, say) at update 300.
		require(impl.execute("record cpu 600").ok && impl.request().restart, "record queues a fresh build");
		drive(director, match, dt, 2000, [&] { return impl.recording() || match.builds >= 2; });
		require(impl.recording() && impl.level_updates() == 0, "Recording starts at a fresh build");
		const int recorded_steps = drive(director, match, dt, 2000, [&] { return !impl.recording(); }, 300);
		require(!impl.recording() && recorded_steps >= length && log_contains(log, "record: saved", DebugSeverity::info) &&
			log_contains(log, "(600 updates, checksum", DebugSeverity::info), "record stops by itself after 600 updates");
		const std::filesystem::path saved = std::filesystem::path(labrador::widen(
			directory_consts::replay_directory())) / L"cpu.json";
		const ReplayFile file = ReplayFile::read(saved);
		require(file.updates == length && file.checkpoints.size() == 10 && file.suspends == std::vector<int>{ 300 } &&
			file.inputs.size() == 4 && file.seed == 7u && file.dt_bits == std::bit_cast<std::uint32_t>(dt),
			"The file holds 600 updates, 10 checkpoints, the suspend, four seats, the seed and dt");
		require(file.final_print.team_a != 0 || file.final_print.team_b != 0, "The bots painted");
		// Recorded seats are post-filter: the keyboard seat as scripted, the bots' own.
		require(file.inputs[0].size() == file.inputs[1].size() && file.inputs[1].size() > 300 &&
			std::any_of(file.inputs[2].begin(), file.inputs[2].end(), [](const PlayerInputData& input)
				{ return input.primary_shoot; }), "Every seat's input is recorded, the bots' included");

		// The same seeded bot match again, unrecorded, ends identically.
		impl.execute("restart");
		drive(director, match, dt, 2000, [&] { return impl.level_updates() >= length; }, 300);
		const std::optional<MatchFingerprint> again = impl.fingerprint();
		require(again && again->hash == file.final_print.hash,
			"The same seed, bots and keys give the same match: " + MatchFingerprint::difference(file.final_print, *again));

		// Replays.
		const auto play = [&](const std::string& line)
			{
				require(impl.execute(line).ok, line + " is accepted");
				require(director.before_match_update(match, dt) && impl.playing(),
					line + " rebuilds the match into the replay");
				require(!impl.clock().paused(), line + " resumes the clock the last replay held");
				drive(director, match, dt, 4000, [&] { return !impl.playing() || impl.level_updates() >= length; });
			};
		// One after another, with no `pause off` between: each ends holding
		// its last frame, and the next play resumes from a paused clock.
		play("play cpu");
		require(log_contains(log, "replay cpu: MATCH (600 updates, 10 checkpoints", DebugSeverity::info),
			"The replay matches the recording");
		require(impl.execute("god on").ok && impl.execute("god off").ok,
			"Once the replay has ended, cheats are accepted again");

		impl.execute("timescale 4");
		play("play cpu");
		impl.execute("timescale 1");
		require(count_of_lines(log, "replay cpu: MATCH") == 2, "At timescale 4 the replay still matches");

		world.resolution.set_resolution(mattmath::Vector2F(1280.0f, 720.0f));
		play("play cpu");
		require(count_of_lines(log, "replay cpu: MATCH") == 3,
			"At another window size the replay still matches (the camera only feeds recorded aim)");
		require(impl.clock().paused() && !log_contains(log, "which the recording was not", DebugSeverity::warning),
			"A held last frame and a paused rebuild are not suspends");

		// A pause the user chose stays, and stepping the replay one update at
		// a time is neutral: the paused clock pauses voices, not players.
		require(impl.execute("pause off").ok && impl.execute("pause on").ok && impl.execute("play cpu").ok &&
			director.before_match_update(match, dt) && impl.playing() && impl.clock().paused(),
			"play keeps a pause the user set");
		const int matched = count_of_lines(log, "replay cpu: MATCH");
		for (int step = 0; step < length + 10 && impl.playing() && !impl.replay_finished(); ++step)
		{
			// What `step 1` (F7) does, without a log line per update.
			impl.clock().request_steps(1);
			director.update_match(match, keyboard_step(step), dt);
		}
		require(count_of_lines(log, "replay cpu: MATCH") == matched + 1 &&
			!log_contains(log, "which the recording was not", DebugSeverity::warning),
			"A replay stepped one update at a time still matches");
		impl.execute("pause off");

		// During a replay the simulation's side doors are shut.
		require(impl.execute("play cpu").ok && director.before_match_update(match, dt) && impl.playing(),
			"play again");
		drive(director, match, dt, 20, [] { return false; });
		require(!impl.execute("god on").ok && !impl.execute("respawn 1").ok &&
			!impl.execute("tune tuning.player.gravity next").ok && !impl.execute("bot all walk").ok &&
			!impl.execute("tune.save").ok && !impl.execute("tune.reset").ok,
			"god, respawn, tune, tune.save, tune.reset and bot are refused while a replay runs");
		require(log_contains(log, "tune.save: replaying 'cpu' would diverge", DebugSeverity::error),
			"tune.save says why");
		require(director.drives_slot(input_slots::keyboard_and_mouse), "A replay drives every recorded slot");
		match.suspend();
		require(log_contains(log, "which the recording was not", DebugSeverity::warning),
			"A suspend the recording lacks is reported as the cause of a divergence");
		require(impl.execute("play stop").ok && !impl.playing(), "play stop");

		// Other tuning: refused, then forced and diverged.
		require(impl.execute("tune tuning.player.gravity " + tune_format(std::stod(
			impl.tune_token("tuning.player.gravity")) + 500.0, impl.tune_token("tuning.player.gravity"))).ok,
			"tune gravity");
		require(!impl.execute("play cpu").ok && log_contains(log, "recorded with other tuning", DebugSeverity::error),
			"A replay with other tuning is refused");
		play("play cpu force");
		require(log_contains(log, "replay cpu: DIVERGED at update", DebugSeverity::error),
			"Forced with other gravity it diverges, at a checkpoint");
		impl.execute("tune.reset");

		// Another step size: refused at the first update. God is on before
		// play (the recording had it off), so the refusal must put it back.
		require(impl.execute("god on").ok && match.god, "god on before the wrong-dt play");
		const bool wrong_dt_played = impl.execute("play cpu").ok;
		const bool wrong_dt_built = director.before_match_update(match, dt);
		require(wrong_dt_played && wrong_dt_built && impl.playing(),
			"play with the wrong dt: " + log.lines().back().text);


		director.update_match(match, keyboard_step(0), 0.0166666f);
		require(!impl.playing() && log_contains(log, "refused: recorded at dt", DebugSeverity::error),
			"A replay at another dt (the executable's StepTimer gives 0.0166666) is refused");
		require(match.god && impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"The refused replay hands back the cheats and the bots, as play stop does: god " +
			std::to_string(match.god) + ", strip '" + impl.status_strip() + "'");
		require(impl.execute("god off").ok && !match.god, "god off");
		require(!impl.execute("play cpu").ok && log_contains(log, "a replay never matches across step sizes",
			DebugSeverity::error), "and play itself refuses once the step is known");
		director.update_match(match, keyboard_step(0), dt);

		// A replay armed and then left behind does not take over the next match.
		require(impl.execute("play cpu").ok && impl.execute("menu").ok &&
			director.before_match_update(match, dt) && !impl.playing() &&
			log_contains(log, "play cpu: disarmed (the match was left)", DebugSeverity::warning),
			"Leaving the match disarms the replay it queued");
		match.build();
		require(!impl.playing() && impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"The next match is a live one, with its bots");

		// From the menus: play starts the match with the recorded settings.
		director.detach_match(match);
		require(impl.execute("play cpu").ok && impl.request().stage == std::optional<std::string>("movement_course"),
			"play in the menus queues the recorded stage");
		FakePage page;
		page.start_result = true;
		require(director.before_page_update(page, dt) && page.starts == 1 &&
			page.started.player_count() == 4 && page.started.stage() == "movement_course" &&
			page.started.workshop_visual_mode() == PlayerVisualMode::authored,
			"and the main menu starts it with the recorded settings");
		CpuMatch fresh(world, director);
		fresh.settings = page.started;
		director.attach_match(fresh);
		fresh.build();
		require(impl.playing(), "The match started from the menus is the replay");
		const int matched_before = count_of_lines(log, "replay cpu: MATCH");
		drive(director, fresh, dt, 4000, [&] { return !impl.playing() || impl.level_updates() >= length; });
		require(count_of_lines(log, "replay cpu: MATCH") == matched_before + 1, "and it matches");
		director.detach_match(fresh);
		impl.execute("bot off");
		impl.execute("seed off");
		std::error_code ignored;
		std::filesystem::remove(saved, ignored);
		return "real Level";
	}
#endif
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: debug_console.exe <game directory>");
		const std::filesystem::path game_directory = argv[1];
		check_executable(game_directory);
		check_launch_detection();

		GameTuning tuning((game_directory / "tuning.json").string().c_str());
		Presentation presentation((game_directory / "presentation.json").string().c_str());

#if COLOURWARS_DEV_TOOLS
		check_tokenizer();
		check_launch_arguments();
		check_registry();
		check_clock();
		check_planner();
		check_text_field();
		check_director(tuning, presentation);
		check_match_hooks(tuning, presentation);
		check_chord();
		check_input_filter(tuning, presentation);
		check_registry_menu_traits(tuning, presentation);
		check_menu_model(tuning, presentation);
		check_unwinding(tuning, presentation);
		check_overlays_and_cheats(tuning, presentation);
		check_goto(tuning, presentation);
		check_screenshot_file(tuning, presentation);
		check_stations(game_directory, tuning);

		// Phase 3. The real content files are only read; replays go to a
		// temporary LOCALAPPDATA.
		const std::filesystem::path source(labrador::widen(source_paths::content_directory));
		const std::string source_tuning = read_text(source / L"tuning.json");
		const std::string source_presentation = read_text(source / L"presentation.json");
		const std::filesystem::path local = std::filesystem::temp_directory_path() /
			(L"colourwars_localappdata_" + std::to_wstring(_getpid()));
		std::filesystem::create_directories(local);
		size_t saved_length = 0;
		wchar_t* saved_raw = nullptr;
		_wdupenv_s(&saved_raw, &saved_length, L"LOCALAPPDATA");
		const std::wstring saved_local = saved_raw != nullptr ? saved_raw : L"";
		std::free(saved_raw);
		_wputenv_s(L"LOCALAPPDATA", local.c_str());
		std::string replay_case;
		try
		{
			check_json_spans(game_directory);
			check_tune_scope(game_directory);
			const int consumed = check_tune_consumed(game_directory);
			std::cout << "NOTE consumed-leaf check: " << consumed << " numbers and booleans, each read "
				"by the game\n";
			check_tune_commands(game_directory);
			check_tune_completion_and_menu(game_directory);
			check_bots(tuning, presentation);
			check_seeded_planner(tuning, presentation);
			check_replay_codec();
			replay_case = check_replay_cpu_match(game_directory);
		}
		catch (...)
		{
			_wputenv_s(L"LOCALAPPDATA", saved_local.c_str());
			throw;
		}
		_wputenv_s(L"LOCALAPPDATA", saved_local.c_str());
		std::error_code ignored;
		std::filesystem::remove_all(local, ignored);
		require(read_text(source / L"tuning.json") == source_tuning &&
			read_text(source / L"presentation.json") == source_presentation,
			"game/content/tuning.json and presentation.json are byte-for-byte unchanged");

		std::cout << "PASS debug console (Debug): tokenizer, launch arguments, registry, "
			"completion, clock, planner, text field, director, match hooks, chord, input "
			"filter, menu traits, menu model, unwinding, overlays and cheats, goto, "
			"screenshot file, stations; phase 3: JSON spans, tune scopes, consumed leaves, "
			"tune/diff/reset/save (CRLF and LF, conflicts), tune completion and menu tree, "
			"bots, seeded planner, replay codec and file, record/play on a " << replay_case <<
			" (MATCH back to back, at timescale 1 and 4, another size and stepped one update "
			"at a time; DIVERGED with other tuning; refused at another dt; disarmed by leaving; "
			"started from the menus); RTTI present.\n";
#else
		DebugDirector director(&tuning, &presentation);
		require(!director.enabled() && director.impl() == nullptr,
			"Release has no director implementation");
		require(!DebugDirector::inert().enabled(), "The inert director is inert");
		const std::string with_statements = captured_stderr([&director]
			{ director.run_launch_arguments({ L"+stage", L"movement_course" }); });
		require(with_statements == "developer arguments ignored: Release build\n",
			"Release prints exactly one stderr line for '+' arguments, got '" +
			with_statements + "'");
		const std::string without = captured_stderr([&director]
			{ director.run_launch_arguments({ L"-x", L"+0.5" }); });
		require(without.empty(), "Release prints nothing without '+' statements, got '" +
			without + "'");

		// The chord, the overlays and the unwinding are inert: pad Back and
		// Start keep F1 and pause, and no page is asked anything.
		std::vector<PlayerInputData> inputs(input_slots::count);
		inputs[0].toggle_debug = true;
		inputs[0].toggle_pause_menu = true;
		director.filter_match_inputs(inputs);
		require(inputs[0].toggle_debug && inputs[0].toggle_pause_menu,
			"Release leaves pad Back and Start alone");
		FakePage page;
		require(!director.before_page_update(page, 1.0f / 60.0f) && page.starts == 0 &&
			page.dismissals == 0 && page.resets == 0, "Release asks no page anything");
		FakeMatch match(director);
		director.attach_match(match);
		director.before_level_build(match);
		director.after_level_build(match);
		require(!director.before_match_update(match, 1.0f / 60.0f), "Release never takes an update");
		director.update_match(match, inputs, 1.0f / 60.0f);
		director.detach_match(match);
		require(match.overlay_sets == 0 && match.player_text_sets == 0 && match.cheat_sets == 0 &&
			match.rebuilds == 0 && match.adopts == 0 && match.placed.empty() &&
			match.respawned.empty() && match.updates == 1,
			"Release pushes no overlay, player text or cheat into a match and runs one update");
		require(match.last_filter == nullptr && match.filtered_updates == 0,
			"Release gives Level::update no seat filter: no bot, recording or replay");
		director.match_suspended(match);
		for (int slot = -1; slot <= input_slots::keyboard_and_mouse; ++slot)
		{
			require(!director.drives_slot(slot), "Release drives no seat, so a missing pad always pauses");
		}

		std::cout << "PASS debug console (Release): inert director; one stderr line for "
			"'+' arguments; pad chord, overlays and unwinding inert; no seat filter and no "
			"driven slot; no console, registry, menu, command, bot, replay or tuning classes "
			"in ColourWars.exe.\n";
#endif
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL debug console: " << error.what() << '\n';
		return 1;
	}
}
