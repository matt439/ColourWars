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
#else
#include "game/input_slots.h"
#include <io.h>
#include <process.h>
#endif

// The developer debug system's CPU-side parts: parsing, the registry, the
// debug clock, the match planner and the line editor; phase 2's chord, debug
// menu model, unwinding, overlays, cheats, goto, screenshot file and level
// stations; in Release, that the build is inert and the executable carries
// none of it. Creates no window, device or save (the PNG check writes and
// removes one temporary file).
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
		void debug_update_level(const std::vector<PlayerInputData>& inputs, float) override
		{
			++this->updates;
			this->jumps.push_back(
				inputs[static_cast<std::size_t>(input_slots::keyboard_and_mouse)].jump_pressed);
		}
		bool debug_level_finished() const override
		{
			return this->finish_after >= 0 && this->updates >= this->finish_after;
		}
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

		bool debug_start_match(const MenuLevelSettings&) override
		{
			++this->starts;
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
			"match", "player", "view", "time", "content", "console" },
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
			"match", "player", "view", "time", "content", "console" },
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
		std::cout << "PASS debug console (Debug): tokenizer, launch arguments, registry, "
			"completion, clock, planner, text field, director, match hooks, chord, input "
			"filter, menu traits, menu model, unwinding, overlays and cheats, goto, "
			"screenshot file, stations; RTTI present.\n";
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
		std::cout << "PASS debug console (Release): inert director; one stderr line for "
			"'+' arguments; pad chord, overlays and unwinding inert; no console, registry, "
			"menu or command classes in ColourWars.exe.\n";
#endif
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL debug console: " << error.what() << '\n';
		return 1;
	}
}
