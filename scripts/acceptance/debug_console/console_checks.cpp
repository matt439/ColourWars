#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_clock.h"
#include "game/debug/debug_keys.h"
#include "game/debug/debug_registry.h"
#include "game/debug/debug_text_field.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/match_plan.h"
#include <memory>
#include <stdexcept>

// Pure-unit checks on the console building blocks: the tokenizer, the launch
// grammar, the registry and completion, the clock, the planner, the text field.
namespace debug_console_acceptance
{
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
			{ L"+stage", L"movement_course", L"+players", L"4", L"+feedback", L"off" });
		require(statements == std::vector<std::string>{
			"stage movement_course", "players 4", "feedback off" },
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

	namespace
	{
	struct FakeGame
	{
		bool feedback = true;
		std::string difficulty = "easy";
		int status_runs = 0;
	};

	void build_registry(DebugRegistry& registry, FakeGame& game)
	{
		registry.add(std::make_unique<BoolVariable>("feedback", "bursts", true,
			[&game] { return game.feedback; },
			[&game](bool value) { game.feedback = value; return DebugStatus::success(); }));
		registry.add(std::make_unique<ChoiceVariable>("difficulty", "test choices", "easy",
			std::vector<std::string>{ "easy", "normal", "hard" },
			std::vector<std::pair<std::string, std::string>>{
				{ "regular", "normal" }, { "expert", "hard" } },
			[&game] { return game.difficulty; },
			[&game](const std::string& value) { game.difficulty = value; return DebugStatus::success(); }));
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
		require(registry.execute("get difficulty", log).ok &&
			log_contains(log, "difficulty = easy  (choice, default easy)", DebugSeverity::info),
			"get prints value, type and default");
		require(registry.execute("difficulty expert", log).ok && game.difficulty == "hard",
			"Aliases map onto their choice");
		require(registry.execute("difficulty next", log).ok && game.difficulty == "easy",
			"next wraps around the choices");
		require(registry.execute("difficulty prev", log).ok && game.difficulty == "hard",
			"prev wraps around the choices");
		require(!registry.execute("difficulty sideways", log).ok && game.difficulty == "hard",
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
		require(registry.complete("difficulty h").line == "difficulty hard ",
			"A choice variable completes its value");
		require(registry.complete("set feed").line == "set feedback ",
			"set completes a variable name");
		require(registry.complete("set difficulty no").line == "set difficulty normal ",
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
		MatchPlan plan = planner.resolve(nullptr, request, no_pads, random);
		require(plan.ok, "A stage and player count resolve: " + plan.error);
		const std::vector<MenuPlayerSettings> seats = plan.settings.player_settings();
		require(seats.size() == 2 && seats[0].num == input_slots::keyboard_and_mouse &&
			seats[1].num == -1, "With no pads, seat 0 is keyboard/mouse and seat 1 an idle dummy");
		require(seats[0].team == PlayerTeam::a && seats[1].team == PlayerTeam::b,
			"Teams alternate a, b");
		require(plan.settings.screen_layout() == labrador::ScreenLayout::two_player,
			"The layout follows the player count");
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
}
#endif
