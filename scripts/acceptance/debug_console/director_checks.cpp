#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_chord.h"
#include "game/debug/debug_screenshot.h"
#include "game/objects/level_loaded_info.h"
#include <chrono>
#include <fstream>
#include <iterator>
#include <objbase.h>
#include <regex>

// The director's match and page hooks against FakeMatch and FakePage: match
// hooks, commands, unwinding, overlays and cheats, goto, the screenshot queue
// and level stations.
namespace debug_console_acceptance
{
	// The director's match hooks, driven against a fake match: what the real
	// step hooks (match_steps to end_match_steps, in GameLevel's step loop),
	// before_match_update, before_level_build, attach_match and detach_match
	// do, not a copy of their logic.
	void check_match_hooks(GameTuning& tuning, Presentation& presentation)
	{
		const StageList stages = {
			{ "movement_course", "Movement Course", "icon", false },
			{ "x", "X", "icon", true },
		};
		const StageCatalog stage_catalog = acceptance::catalog_of(stages);
		GameData data;
		data.set_stage_catalog(&stage_catalog);
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
		require(impl.execute("players 4").ok && impl.execute("weapon all sniper").ok,
			"Menu-time settings are accepted");
		director.attach_match(match);
		require(impl.request().empty(), "attach_match drops menu-time leftovers");
		require(log_contains(impl.log(), "dropped queued players 4", DebugSeverity::warning),
			"Dropping a leftover request is logged");
		require(!director.before_match_update(match, dt) && match.rebuilds == 0 &&
			match.settings.player_count() == 2,
			"A match started from the menus is not rebuilt from menu-time settings");

		const std::size_t kbm = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
		std::vector<SlotInput> tap(input_slots::count);
		tap[kbm].jump_pressed = true;
		tap[kbm].jump_held = true;
		const std::vector<SlotInput> none(input_slots::count);

		// Timescale 1: one update, the edge delivered.
		update_match(director, match, tap, dt);
		require(match.updates == 1 && match.jumps_delivered() == 1,
			"Timescale 1 runs one update with the press");

		// Slow motion: a press on a step with no update is delivered once, later.
		match.reset_counts();
		impl.execute("timescale 0.25");
		impl.clock().reset_accumulator();
		for (int step = 0; step < 8; ++step)
		{
			update_match(director, match, step == 0 ? tap : none, dt);
		}
		require(match.updates == 2, "Timescale 0.25 runs two updates in eight steps, got " +
			std::to_string(match.updates));
		require(match.jumps_delivered() == 1 && match.jumps.front(),
			"A press during slow motion is delivered exactly once, on the next update");

		// Fast forward: four updates, the edge only on the first.
		match.reset_counts();
		impl.execute("timescale 4");
		update_match(director, match, tap, dt);
		require(match.updates == 4 && match.jumps_delivered() == 1 && match.jumps.front(),
			"Timescale 4 runs four updates and delivers the press once");

		// A round that ends mid fast-forward stops there.
		match.reset_counts();
		match.finish_after = 2;
		update_match(director, match, none, dt);
		require(match.updates == 2, "A level that finishes mid fast-forward is not updated again");
		match.reset_counts();
		impl.execute("timescale 1");

		// Paused: nothing runs; a step runs one update and resyncs audio.
		impl.execute("pause on");
		update_match(director, match, tap, dt);
		require(match.updates == 0, "A paused clock runs no update");
		const int syncs = match.audio_syncs;
		require(impl.execute("step 1").ok, "step works in a match");
		update_match(director, match, none, dt);
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
		require(impl.registry().find("visual") == nullptr, "The retired art-mode command is absent");
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

		require(impl.execute("bind ctrl+j feedback toggle").ok &&
			!director.keyboard_jump_collision(labrador::Key::j),
			"A modified developer bind does not collide with a bare Jump key");
		require(impl.execute("bind j feedback toggle").ok &&
			director.keyboard_jump_collision(labrador::Key::j) &&
			!director.keyboard_jump_collision(labrador::Key::f10),
			"A bare canonical Jump candidate reports its developer collision without admitting reserved keys");
		require(impl.execute("unbind j").ok && !director.keyboard_jump_collision(labrador::Key::j),
			"Removing the bare bind clears the Jump collision while its modified bind remains");

		DebugChord consumed;
		DebugChord::Pads pads{};
		pads[1] = { true, true, true };
		require(!consumed.update(pads, 0.2f), "A partial debug chord has not fired");
		consumed.consume(pads);
		require(!consumed.update(pads, 2.0f) && consumed.suppressing(1),
			"Capture consumes a held debug chord rather than deferring its toggle");
		pads[1].back = false;
		require(!consumed.update(pads, 1.0f) && consumed.suppressing(1),
			"A consumed chord stays suppressed until both buttons release");
		pads[1].start = false;
		require(!consumed.update(pads, 0.01f) && consumed.suppressing(1),
			"The consumed chord's final release still belongs to it");
		require(!consumed.update(pads, 0.01f) && !consumed.suppressing(1),
			"A fully released consumed chord rearms on the next neutral frame");
		pads[1].back = pads[1].start = true;
		require(consumed.update(pads, DebugChord::hold_seconds),
			"A fresh chord still opens the developer menu after capture");
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

		// Raw capture cancels queued work rather than replaying it on release.
		director.attach_match(match);
		require(impl.execute("players 4").ok && impl.execute("menu").ok &&
			impl.execute("screenshot capture-cancelled").ok && impl.execute("goto high_step").ok,
			"Capture cancellation fixture queues a rebuild, leave, screenshot and goto");
		DebugScriptRunner::Frame script;
		script.name = "capture-cancelled.cfg";
		script.statements.push_back({ script.name, 1, "stage x" });
		require(impl.scripts().push(std::move(script)).ok, "Capture cancellation fixture queues a future script");
		const float speed = impl.clock().timescale();
		const auto binds = impl.binds();
		const int prior_leaves = match.leaves, prior_rebuilds = match.rebuilds;
		impl.log().clear();
		director.begin_keyboard_capture();
		require(impl.request().empty() && !impl.scripts().running() && !impl.pending_screenshot() &&
			impl.clock().timescale() == speed && impl.binds() == binds &&
			impl.log().lines().size() == 1 &&
			log_contains(impl.log(), "discarded queued developer work", DebugSeverity::warning),
			"Capture atomically cancels queued developer work with one notice while preserving settings");
		FakePage capturing;
		director.capture_input_frame(capturing);
		require(!director.before_page_update(capturing, dt) && capturing.starts == 0 && capturing.dismissals == 0 &&
			impl.log().lines().size() == 1,
			"Repeated capture frames run no developer page work or duplicate cancellation notice");
		director.end_keyboard_capture();
		require(!director.before_page_update(capturing, dt) &&
			!director.before_match_update(match, dt) && match.leaves == prior_leaves && match.rebuilds == prior_rebuilds,
			"Capture release does not replay the discarded page, leave or rebuild requests");
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
		require(impl.execute("kill 1").ok && match.killed == std::vector<int>{ 0 } &&
			!impl.execute("kill 2").ok && log_contains(impl.log(),
				"kill: seat 2 is not in this match", DebugSeverity::error),
			"kill splats a seat in the match and names one that is not");

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
		// DBG13: setter-only validation on an application-free director. Never
		// queue a screenshot or open a proposed folder, including device names.
		{
			TestDirector directory_test(tuning, presentation);
			DebugDirectorImpl& directories = directory_test.impl;
			require(!directories.user_directory().has_value() && !directories.pending_screenshot().has_value(),
				"DBG13 setter-only director unexpectedly has a profile or queued capture");
			const auto query = [&](const std::string& expected, bool prefix = false)
			{
				directories.log().clear();
				const DebugStatus result = directories.execute("screenshot.dir");
				const auto& lines = directories.log().lines();
				require(result.ok && !result.close_console && lines.size() == 1 && lines.front().severity == DebugSeverity::info &&
					(prefix ? lines.front().text.starts_with(expected) : lines.front().text == expected),
					"DBG13 current-directory query did not emit its expected fresh info line");
			};
			const auto accept = [&](const std::string& folder, const std::string& expected)
			{
				const DebugStatus result = directories.execute("screenshot.dir \"" + folder + "\"");
				require(result.ok && !result.close_console && directories.screenshot_dir() == expected &&
					!directories.pending_screenshot().has_value(), "DBG13 valid folder changed: " + folder);
				query("screenshot.dir = " + expected);
			};
			const auto refuse = [&](const std::string& folder, const std::string& message)
			{
				const std::string previous = directories.screenshot_dir();
				const DebugStatus result = directories.execute("screenshot.dir \"" + folder + "\"");
				require(!result.ok && !result.close_console && result.message == message &&
					directories.screenshot_dir() == previous && !directories.pending_screenshot().has_value(),
					"DBG13 screenshot.dir did not refuse '" + folder + "' immediately while retaining its prior folder: " + result.message);
				query("screenshot.dir = " + previous);
			};
			accept("retained/session", "retained/session");
			// First new assertion: the old setter succeeds, without accessing disk.
			refuse("CON", "screenshot.dir: reserved Windows folder name: CON");
			std::vector<std::string> reserved{ "CON", "PRN", "AUX", "NUL" };
			for (int digit = 1; digit <= 9; ++digit)
			{
				reserved.push_back("COM" + std::to_string(digit));
				reserved.push_back("LPT" + std::to_string(digit));
			}
			std::size_t refusals = 1;
			for (const std::string& stem : reserved)
				for (int spelling = 0; spelling < 3; ++spelling)
				{
					std::string name = stem;
					for (std::size_t index = 0; index < name.size(); ++index)
						if ((spelling == 1 || (spelling == 2 && index % 2 == 1)) && name[index] >= 'A' && name[index] <= 'Z')
							name[index] = static_cast<char>(name[index] - 'A' + 'a');
					for (char slash : { '/', '\\' })
						for (int position = 0; position < 3; ++position)
						{
							const std::string separator(1, slash);
							const std::string folder = position == 0 ? name + separator + "safe" + separator + "last"
								: position == 1 ? "safe" + separator + name + separator + "last"
								: "safe" + separator + "middle" + separator + name;
							refuse(folder, "screenshot.dir: reserved Windows folder name: " + name);
							++refusals;
						}
				}
			require(refusals == 397, "DBG13 reserved-name matrix did not visit every spelling, separator and position");
			for (const char* name : { "COM0", "COM10", "LPT0", "LPT10", "CON_", "NULx", "PRNx", "_AUX", "-review" })
				accept(name, name);
			const std::string long_name(96, 'q');
			accept(long_name, long_name);
			accept("safe/second/third", "safe/second/third");
			accept("safe\\second\\third", "safe/second/third");
			accept("/safe//./second/", "safe/second");
			accept("two words/next?", "two_words/next_");
			accept("CON.txt", "CON_txt");
			accept("CON.", "CON_");
			const DebugStatus timed = directories.execute("screenshot.dir review-{time}/frame");
			require(timed.ok && !timed.close_console && !directories.pending_screenshot().has_value() &&
				std::regex_match(directories.screenshot_dir(), std::regex("review-[0-9]{8}-[0-9]{6}/frame")),
				"DBG13 screenshot directory lost its existing local-time expansion");
			accept("retained/session", "retained/session");
			refuse("a/../b", "screenshot.dir: '..' is not allowed");
			refuse("a/b/c/d", "screenshot.dir: at most three folders");
			refuse("a/../b/c/d/e", "screenshot.dir: at most three folders");
			refuse("CON/../x", "screenshot.dir: '..' is not allowed");
			refuse("../CON", "screenshot.dir: '..' is not allowed");
			refuse("CON/a/b/c", "screenshot.dir: at most three folders");
			refuse("a/b/CON/d", "screenshot.dir: at most three folders");
			refuse("/./", "screenshot.dir: no folder name in '/./'");
			for (const char* command : { "screenshot.dir off", "screenshot.dir \"\"" })
			{
				directories.log().clear();
				const DebugStatus disabled = directories.execute(command);
				const auto& lines = directories.log().lines();
				require(disabled.ok && !disabled.close_console && directories.screenshot_dir().empty() &&
					lines.size() == 1 && lines.front().severity == DebugSeverity::info &&
					lines.front().text.starts_with("screenshot.dir off: ") && !directories.pending_screenshot().has_value(),
					"DBG13 screenshot directory lost off/empty/query behavior");
				query("screenshot.dir = off (", true);
			}
			require(!directories.user_directory().has_value(), "DBG13 setter-only validation selected a player profile");
		}

		const auto now = std::chrono::system_clock::now();
		// DBG14: pure filename values only; never open a reserved device path.
		{
			const std::string first = screenshot_file_name("CON", now);
			require(first == "_CON.png", "DBG14 reserved screenshot basename was not protected: " + first);
			std::vector<std::string> reserved{ "CON", "PRN", "AUX", "NUL" };
			for (int digit = 1; digit <= 9; ++digit)
			{
				reserved.push_back("COM" + std::to_string(digit));
				reserved.push_back("LPT" + std::to_string(digit));
			}
			std::size_t protected_names = 1;
			for (const std::string& stem : reserved)
				for (int spelling = 0; spelling < 3; ++spelling)
				{
					std::string name = stem;
					for (std::size_t index = 0; index < name.size(); ++index)
						if ((spelling == 1 || (spelling == 2 && index % 2 == 1)) && name[index] >= 'A' && name[index] <= 'Z')
							name[index] = static_cast<char>(name[index] - 'A' + 'a');
					for (const char* suffix : { "", ".png", ".PnG" })
					{
						const std::string requested = name + suffix;
						const std::string actual = screenshot_file_name(requested, now);
						require(actual == "_" + name + ".png", "DBG14 reserved basename changed: " + requested + " -> " + actual);
						++protected_names;
					}
				}
			require(protected_names == 199, "DBG14 reserved matrix did not visit every spelling and suffix");
			for (const char* neighbor : { "COM0", "COM10", "LPT0", "LPT10", "CON_", "_CON", "-CON", "CONSOLE" })
				for (const char* suffix : { "", ".png", ".PnG" })
				{
					const std::string name(neighbor);
					require(screenshot_file_name(name + suffix, now) == name + ".png",
						"DBG14 valid neighboring basename changed: " + name + suffix);
				}
			require(screenshot_file_name("CON.", now) == "CON_.png" &&
				screenshot_file_name("CON.txt", now) == "CON_txt.png" &&
				screenshot_file_name("dir/CON", now) == "dir_CON.png" &&
				screenshot_file_name(".png", now) == "_png.png",
				"DBG14 filename protection changed existing suffix or punctuation sanitization");
			std::printf("PASS DBG14 screenshot basenames: 199 reserved stem/case/suffix values, 24 valid neighbors, "
				"sanitized punctuation/path controls; pure strings, no capture or reserved-path access\n");
		}

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
			"workshop", "refill", "spawn_a", "spawn_b" }, "The course lists its ten stations, then the spawns");
		require(stations[10].position == info.team_a_spawns()[0] &&
			stations[11].position == info.team_b_spawns()[0], "spawn_a and spawn_b are the first spawns");

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
		for (std::size_t i = 0; i < 10; ++i)
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
}
#endif
