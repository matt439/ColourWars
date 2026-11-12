#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"
#include "scripts/acceptance/debug_console/cpu_level.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay/timelapse.h"
#include "game/objects/match_random.h"

// replay.timelapse (REV#48): the command's arming, refusals, page walk, stills
// schedule and drops against FakeMatch and FakePage, and Level's arena view on
// a real CPU Level, where a whole tape played many steps a frame through that
// view ends on the round's own paint. GameLevel's own use of both (the End
// menu's Watch Replay, the silent pass, the banner) needs a window and is the
// painter's to see.
namespace debug_console_acceptance
{
	void check_timelapse_command(GameTuning& tuning, Presentation& presentation)
	{
		// The stills are evenly spaced, the last on the final step.
		require(timelapse::still_step(0, 4, 1000) == 250 && timelapse::still_step(1, 4, 1000) == 500 &&
			timelapse::still_step(3, 4, 1000) == 1000 && timelapse::still_step(0, 3, 10) == 4 &&
			timelapse::still_step(2, 3, 10) == 10 && timelapse::still_step(0, 0, 10) == 0,
			"replay.timelapse spaces its stills evenly, the last on the final step");
		require(timelapse::still_name("20261112-101010-007", 6) == "timelapse-20261112-101010-007-07.png" &&
			timelapse::still_name("s", 11) == "timelapse-s-12.png",
			"replay.timelapse names its stills timelapse-<stamp>-NN.png");

		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		DebugDirector& director = test.director;
		const float dt = 1.0f / 60.0f;
		const auto lines_with = [&impl](const std::string& text)
			{
				return std::count_if(impl.log().lines().begin(), impl.log().lines().end(),
					[&text](const DebugLogLine& line) { return line.text.find(text) != std::string::npos; });
			};

		require(!impl.execute("replay.timelapse").ok, "replay.timelapse is refused with no match");
		require(!impl.execute("replay.timelapse off").ok, "replay.timelapse off is refused with none armed");

		FakeMatch match(director);
		match.settings = planned(2);
		director.attach_match(match);
		match.round_taped = false;
		require(!impl.execute("replay.timelapse").ok && match.armed_speed == -1,
			"replay.timelapse is refused for a round with no playable tape");
		match.round_taped = true;
		for (const char* bad : { "replay.timelapse 0", "replay.timelapse 65", "replay.timelapse fast",
			"replay.timelapse 8 -1", "replay.timelapse 8 33", "replay.timelapse 8 many" })
		{
			require(!impl.execute(bad).ok && match.armed_speed == -1, std::string("Refused: ") + bad);
		}

		// Armed during the round: it waits for the round's pages.
		impl.log().clear();
		const DebugStatus armed = impl.execute("replay.timelapse");
		require(armed.ok && armed.close_console && match.armed_speed == timelapse::default_speed &&
			impl.timelapse_pending() && impl.wait_context().timelapse_active &&
			log_contains(impl.log(), "starts when the round's End menu opens", DebugSeverity::info),
			"replay.timelapse arms the match at the default speed and waits for the End menu");
		require(!impl.execute("replay.timelapse 4").ok && match.armed_speed == timelapse::default_speed,
			"A second replay.timelapse is refused while one is armed");
		FakePage page;
		for (int frame = 0; frame < 3; ++frame)
		{
			require(!director.before_page_update(page, dt), "A page that cannot reach Watch Replay keeps its frame");
		}
		require(page.watch_requests == 3 && page.dismissals == 0 && impl.timelapse_pending() &&
			lines_with("queued: replay.timelapse") == 1,
			"An armed time-lapse asks each page, dismisses none and says it waits once");

		// The End menu takes it: GameLevel then plays frames and reports them.
		page.watch_result = true;
		require(director.before_page_update(page, dt) && page.watch_requests == 4,
			"The page that steps toward Watch Replay takes the frame");
		match.timelapse_running = true;
		director.timelapse_frame(match, 16, 100);
		require(!impl.timelapse_pending() && impl.wait_context().timelapse_active &&
			log_contains(impl.log(), "replay.timelapse: playing 100 taped steps at x16", DebugSeverity::info),
			"The first frame starts the time-lapse");
		require(!impl.execute("replay.timelapse off").ok, "replay.timelapse off is refused while it plays");
		director.timelapse_frame(match, 100, 100);
		require(!impl.wait_context().timelapse_active && log_contains(impl.log(),
			"replay.timelapse: done, 100 taped steps at x16, 0 of 0 still(s) saved", DebugSeverity::info),
			"The last step ends the time-lapse");
		match.timelapse_running = false;

		// Stills: with no window none is written, but each is tried once, when due.
		impl.log().clear();
		page.watch_result = true;
		require(impl.execute("replay.timelapse 32 3").ok && match.armed_speed == 32,
			"replay.timelapse takes a speed and a number of stills");
		require(director.before_page_update(page, dt), "The End menu takes the armed time-lapse");
		match.timelapse_running = true;
		director.timelapse_frame(match, 32, 96);
		director.timelapse_frame(match, 64, 96);
		director.timelapse_frame(match, 96, 96);
		require(!impl.wait_context().timelapse_active && log_contains(impl.log(),
			"replay.timelapse: done, 96 taped steps at x32, 0 of 3 still(s) saved", DebugSeverity::info),
			"A run that could write no still says so");

		// Ended early: a press ends GameLevel's pass and the End menu is back.
		impl.log().clear();
		require(impl.execute("replay.timelapse 8").ok && director.before_page_update(page, dt),
			"Armed again and taken by the End menu");
		director.timelapse_frame(match, 8, 100);
		match.timelapse_running = false;
		require(!director.before_page_update(page, dt) && !impl.wait_context().timelapse_active &&
			log_contains(impl.log(), "replay.timelapse: ended early", DebugSeverity::info),
			"A time-lapse ended by a press is over when a page is back on top");

		// Drops: off, a rebuild, raw keyboard capture, a spoiled tape at the End menu, the match ending.
		page.watch_result = false;
		require(impl.execute("replay.timelapse 8").ok && impl.execute("replay.timelapse off").ok &&
			!impl.wait_context().timelapse_active && match.armed_speed == 0,
			"replay.timelapse off disarms the match");
		require(impl.execute("replay.timelapse 8").ok, "Armed before a rebuild");
		director.before_level_build(match);
		require(!impl.wait_context().timelapse_active && match.armed_speed == 0 &&
			log_contains(impl.log(), "replay.timelapse: disarmed (the match was rebuilt)", DebugSeverity::warning),
			"A rebuild drops an armed time-lapse");
		// Raw keyboard capture with only the time-lapse queued still drops it.
		require(impl.execute("replay.timelapse 8").ok, "Armed before raw keyboard capture");
		impl.log().clear();
		director.begin_keyboard_capture();
		director.end_keyboard_capture();
		require(!impl.wait_context().timelapse_active && match.armed_speed == 0 &&
			log_contains(impl.log(), "replay.timelapse: disarmed (raw keyboard capture)", DebugSeverity::warning),
			"Raw keyboard capture drops an armed time-lapse that is the only queued work");
		require(impl.execute("replay.timelapse 8").ok, "Armed before the round ends spoiled");
		match.finish_after = 0;
		match.round_taped = false;
		require(!director.before_page_update(page, dt) && !impl.wait_context().timelapse_active &&
			log_contains(impl.log(), "disarmed (the round's tape cannot be replayed)", DebugSeverity::warning),
			"A finished round with no playable tape drops the time-lapse");
		match.round_taped = true;
		match.finish_after = -1;
		require(impl.execute("replay.timelapse 8").ok, "Armed before the match ends");
		director.detach_match(match);
		require(!impl.wait_context().timelapse_active &&
			log_contains(impl.log(), "disarmed (the match ended)", DebugSeverity::warning),
			"Leaving the match drops an armed time-lapse");

		std::cout << "PASS replay.timelapse: arming, refusals, the page walk, stills schedule, "
			"done/ended-early and every drop\n";
	}

	void check_timelapse_level(const std::filesystem::path& game_directory)
	{
		GameTuning tuning((game_directory / L"tuning.json").string().c_str());
		Presentation presentation((game_directory / L"presentation.json").string().c_str());
		CpuWorld world(game_directory, tuning, presentation);
		const float dt = 1.0f / 60.0f;

		MatchRequest roster;
		roster.stage = "close_quarters";
		roster.players = 2;
		MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [] { return 0u; });
		require(plan.ok, "The time-lapse roster resolves");
		MatchSetup settings = plan.settings;
		// Seat 2 a shipping bot, which paints, beside the scripted keyboard seat.
		settings.set_player_bot(1, BotDifficulty::normal);
		settings.set_screen_layout(layout_for_humans(settings.human_count()));
		settings.set_duration(4.0f);
		const unsigned int seed = 48;

		// The round, taped as GameLevel tapes it, with a scripted keyboard seat.
		world.viewports.set_layout(settings.screen_layout());
		RoundTape tape;
		RoundTapeRecorder recorder(&tape);
		tape.start(seed, settings);
		std::unique_ptr<Level> round = world.builder->build_level(settings, TeamColourPair::blue_orange,
			OpeningMode::off, MatchRandom(seed));
		for (int tick = 0; tick < 20000 && round->state() != LevelState::zoom_out; ++tick)
		{
			round->refresh_layout();
			const SeatCommands commands = world.seat_commands.build(*round, keyboard_step(tick));
			recorder.prepare(nullptr, dt);
			round->update(commands.seats, dt, &recorder);
		}
		require(round->state() == LevelState::zoom_out && tape.playable() && tape.steps() > 100,
			"The taped round reaches time up");
		const LevelEndInfo played = round->level_end_info();
		require(played.team_a_area + played.team_b_area > 0.0f, "The taped round painted something");
		round.reset();

		// The time-lapse: built again from the tape, in the arena view.
		std::unique_ptr<Level> shown = world.builder->build_level(tape.settings(), TeamColourPair::blue_orange,
			OpeningMode::off, MatchRandom(tape.seed()));
		shown->set_player_cheats(tape.damage_immune(), tape.infinite_ammo());
		shown->start_without_countdown();
		shown->refresh_layout();
		const int panes = shown->scene_view_count();
		require(panes == shown->view_count() && !shown->arena_view(), "Live play draws one pane per human");
		shown->set_arena_view(true);
		const auto one_arena_view = [&](const Level& level)
			{
				return level.scene_view_count() == 1 && level.arena_view() &&
					level.scene_view(0).camera == level.overview_camera() &&
					level.scene_view(0).viewport == world.viewports.fullscreen_viewport();
			};
		require(one_arena_view(*shown), "The arena view is one fullscreen view through the overview camera");

		RoundTapePlayer player(&tape, 0);
		std::vector<PlayerInputData> neutral(input_slots::count);
		for (PlayerInputData& device : neutral) { device.connected = true; }
		int frames = 0;
		while (!player.finished())
		{
			for (int run = 0; run < timelapse::default_speed && !player.finished(); ++run)
			{
				const int at = player.step();
				tape.apply_gaps(*shown, at);
				shown->update(neutral, tape.dt(at), &player);
			}
			shown->refresh_layout();
			require(one_arena_view(*shown), "Every time-lapse frame keeps the one arena view");
			++frames;
		}
		require(frames == (tape.steps() + timelapse::default_speed - 1) / timelapse::default_speed,
			"The time-lapse plays its speed's worth of taped steps a frame");
		const LevelEndInfo lapsed = shown->level_end_info();
		require(lapsed.total_area == played.total_area && lapsed.team_a_area == played.team_a_area &&
			lapsed.team_b_area == played.team_b_area && lapsed.player_areas == played.player_areas,
			"The time-lapse ends on the round's own paint: the arena view changes only the drawing");
		shown->set_arena_view(false);
		require(!shown->arena_view() && shown->scene_view_count() == 1,
			"Off, the finished level draws its own overview view");

		std::cout << "PASS replay.timelapse level: one arena view through the overview camera, " <<
			tape.steps() << " taped steps in " << frames << " frames at x" << timelapse::default_speed <<
			", the round's own paint\n";
	}
}
#endif
