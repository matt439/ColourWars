#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "game/debug/debug_director.h"
#include "game/debug/debug_tokenizer.h"
#include "game/presentation.h"
#include "game/tuning.h"

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
#include "game/debug/debug_log.h"
#include "game/debug/debug_registry.h"
#include "game/debug/debug_text_field.h"
#include "game/debug/match_plan.h"
#include "game/game_data.h"
#include "game/input_slots.h"
#include <algorithm>
#else
#include <io.h>
#include <process.h>
#endif

// The developer debug system's CPU-side parts: parsing, the registry, the
// debug clock, the match planner and the line editor in Debug; in Release,
// that the build is inert and the executable carries none of it. Creates no
// window, device or save.
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

	// A match as the director sees it: counts updates and rebuilds, records
	// each update's jump edge, and can finish after a number of updates.
	// Like GameLevel, its rebuild goes through before_level_build.
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

		const MenuLevelSettings& debug_settings() const override { return this->settings; }
		void debug_rebuild(const MenuLevelSettings& plan, bool reset_session) override
		{
			this->settings = plan;
			++this->rebuilds;
			this->last_reset_session = reset_session;
			this->director_.before_level_build();
		}
		void debug_leave() override { ++this->leaves; }
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
		require(!director.before_match_update(match) && match.rebuilds == 0 &&
			match.settings.player_count() == 2,
			"A match started from the menus is not rebuilt from menu-time settings");

		const std::size_t kbm = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
		std::vector<PlayerInputData> tap(input_slots::count);
		tap[kbm].jump_pressed = true;
		tap[kbm].jump_held = true;
		const std::vector<PlayerInputData> none(input_slots::count);
		const float dt = 1.0f / 60.0f;

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
		require(director.before_match_update(match) && match.rebuilds == 1 &&
			!match.last_reset_session && match.settings.player_count() == 2,
			"restart rebuilds once with the current settings and keeps the session");
		require(impl.request().empty(), "A rebuild consumes the request");

		// A roster change rebuilds with the planned settings.
		require(impl.execute("players 3").ok && impl.execute("weapon 2 roller").ok,
			"In a match, players and weapon queue a rebuild");
		require(director.before_match_update(match) && match.rebuilds == 2 &&
			match.last_reset_session && match.settings.player_count() == 3 &&
			match.settings.player_weapon(1) == WeaponType::roller,
			"debug_rebuild receives the planned roster and weapons");

		// A restart from the pause or end menu is the restart: its build
		// consumes the queued one, so the match is not built twice.
		require(impl.execute("restart").ok, "restart queues");
		director.before_level_build();
		require(impl.request().empty() && !director.before_match_update(match) &&
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
		std::cout << "PASS debug console (Debug): tokenizer, launch arguments, registry, "
			"completion, clock, planner, text field, director, match hooks; RTTI present.\n";
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
		std::cout << "PASS debug console (Release): inert director; one stderr line for "
			"'+' arguments; no console, registry or command classes in ColourWars.exe.\n";
#endif
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL debug console: " << error.what() << '\n';
		return 1;
	}
}
