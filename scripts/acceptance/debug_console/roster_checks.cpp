#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "game/debug/replay.h"
#include "game/directory_consts.h"
#include "scripts/acceptance/debug_console/cpu_level.h"
#include <process.h>
#include <sstream>

// DBG#17: `seat <n> team|bot|human|boost` statements, planned by MatchPlanner
// and recorded and replayed through format 4 unchanged.
namespace debug_console_acceptance
{
	namespace
	{
		constexpr float roster_step = 1.0f / 60.0f;
		constexpr int roster_length = 240;

		// Every field a plan sets, in seat order, for byte comparisons.
		std::string roster_text(const MatchSetup& settings)
		{
			std::ostringstream out;
			out << settings.stage() << '|' << static_cast<int>(settings.screen_layout());
			for (const SeatSetup& seat : settings.player_settings())
			{
				out << '|' << seat.num << ':' << static_cast<int>(seat.team) << ':' <<
					static_cast<int>(seat.weapon) << ':' << static_cast<int>(seat.controller) << ':' <<
					static_cast<int>(seat.difficulty) << ':' << seat.handicap;
			}
			for (const int device : settings.pad_devices()) { out << '|' << device; }
			return out.str();
		}

		// The roster as a replay stores it: a human's difficulty reads back normal.
		std::string recorded_roster(const MatchSetup& settings)
		{
			std::ostringstream out;
			out << settings.stage() << '|' << static_cast<int>(settings.screen_layout());
			for (const SeatSetup& seat : settings.player_settings())
			{
				out << '|' << seat.num << ':' << static_cast<int>(seat.team) << ':' <<
					static_cast<int>(seat.weapon) << ':' << static_cast<int>(seat.controller) << ':' <<
					static_cast<int>(seat.controller == PlayerController::bot ? seat.difficulty :
						BotDifficulty::normal) << ':' << seat.handicap;
			}
			return out.str();
		}

		MatchRequest::SeatChoice seat_choice(int seat, const std::string& field, const std::string& value)
		{
			MatchRequest::SeatChoice choice;
			const std::string error = MatchPlanner::parse_seat_choice(field, value, choice);
			require(error.empty(), "Seat fixture does not parse: " + error);
			choice.seat = seat;
			return choice;
		}

		void roster_tick(DebugDirector& director, CpuMatch& match)
		{
			if (!director.before_match_update(match, roster_step))
			{
				update_match(director, match, keyboard_step(director.impl()->level_updates()), roster_step);
			}
		}
	}

	void check_roster_planner()
	{
		StageList stages = {
			{ "movement_course", "Movement Course", "icon", false },
			{ "x", "X", "icon", true },
		};
		const MatchPlanner planner(&stages);
		const MatchPlanner::ConnectedPads no_pads{};
		const MatchPlanner::ConnectedPads all_pads{ true, true, true, true };
		const MatchPlanner::Random random = [] { return 0u; };

		// Existing plans: every seat human at Normal with no Boost, parity
		// teams, the player-count layout, unchanged by the new fields.
		for (const MatchPlanner::ConnectedPads& pads : { no_pads, all_pads })
		{
			for (int players = 1; players <= 4; ++players)
			{
				MatchRequest request;
				request.stage = "movement_course";
				request.players = players;
				const MatchPlan plan = planner.resolve(nullptr, request, pads, random);
				require(plan.ok && plan.settings.screen_layout() == MatchPlanner::layout_for(players) &&
					plan.settings.human_count() == players, "A players plan changed its layout or humans");
				for (int seat = 0; seat < players; ++seat)
				{
					const SeatSetup& settings = plan.settings.player_settings()[static_cast<std::size_t>(seat)];
					require(settings.controller == PlayerController::human &&
						settings.difficulty == BotDifficulty::normal && settings.handicap == 0 &&
						settings.team == (players == 1 || seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b),
						"A players plan changed a seat's controller, difficulty, Boost or team");
				}
				// The devices assign_devices gave before seats existed: every
				// pad in slot order, or keyboard/mouse then idle dummies.
				for (int seat = 0; seat < players; ++seat)
				{
					const int expected = pads[0] ? seat : seat == 0 ? input_slots::keyboard_and_mouse : -1;
					require(plan.settings.player_settings()[static_cast<std::size_t>(seat)].num == expected,
						"A players plan moved a seat's device: " + roster_text(plan.settings));
				}
				require(plan.settings.pad_devices() == std::array<int, input_slots::pad_count>{ 0, 1, 2, 3 },
					"A players plan moved a pad onto another slot");
			}
		}

		// Every field, with every pad connected: bots take no device and the
		// humans take pads in slot order.
		MatchRequest every;
		every.stage = "movement_course";
		every.players = 4;
		every.seats = { seat_choice(1, "bot", "easy"), seat_choice(1, "boost", "2"),
			seat_choice(1, "team", "a"), seat_choice(2, "bot", "hard"), seat_choice(2, "team", "a"),
			seat_choice(0, "team", "b"), seat_choice(3, "boost", "1"), seat_choice(0, "boost", "1") };
		MatchPlan plan = planner.resolve(nullptr, every, all_pads, random);
		require(plan.ok, "A plan with every seat field resolves: " + plan.error);
		const std::vector<SeatSetup> seats = plan.settings.player_settings();
		require(seats[0].controller == PlayerController::human && seats[0].num == 0 &&
			seats[0].team == PlayerTeam::b && seats[0].handicap == 1, "Seat 1: pad 0, team b, Boost 1");
		require(seats[1].controller == PlayerController::bot && seats[1].num == -1 &&
			seats[1].difficulty == BotDifficulty::easy && seats[1].team == PlayerTeam::a &&
			seats[1].handicap == 2, "Seat 2: an Easy bot with no device, team a, Boost 2");
		require(seats[2].controller == PlayerController::bot && seats[2].num == -1 &&
			seats[2].difficulty == BotDifficulty::hard && seats[2].team == PlayerTeam::a &&
			seats[2].handicap == 0, "Seat 3: a Hard bot with no device, team a");
		require(seats[3].controller == PlayerController::human && seats[3].num == 1 &&
			seats[3].team == PlayerTeam::b && seats[3].handicap == 1,
			"Seat 4: the next pad, parity team b, Boost 1");
		require(plan.settings.human_count() == 2 &&
			plan.settings.screen_layout() == labrador::ScreenLayout::two_player,
			"Bots have no pane: the layout follows the humans");
		require(plan.reset_session, "A seat statement starts a new session");

		// On the running match: later statements win, teams are kept, and a
		// bot handed back to a human takes a device again.
		MatchRequest later;
		later.seats = { seat_choice(1, "bot", "normal"), seat_choice(1, "bot", "hard"),
			seat_choice(2, "human", ""), seat_choice(3, "boost", "0") };
		const MatchPlan changed = planner.resolve(&plan.settings, later, all_pads, random);
		require(changed.ok, "Seat statements resolve on a running match: " + changed.error);
		const std::vector<SeatSetup> now = changed.settings.player_settings();
		require(now[1].controller == PlayerController::bot && now[1].difficulty == BotDifficulty::hard &&
			now[1].team == PlayerTeam::a && now[1].handicap == 2,
			"A later difficulty wins and keeps the bot's team and Boost");
		require(now[2].controller == PlayerController::human && now[2].num == 1 && now[3].num == 2 &&
			now[2].team == PlayerTeam::a && now[3].handicap == 0 &&
			changed.settings.screen_layout() == labrador::ScreenLayout::three_player,
			"A seat made human takes a pad in seat order and gets a pane");
		require(changed.settings.stage() == "movement_course" && changed.settings.player_count() == 4,
			"Seat statements keep the stage and the count");

		// A players change after seat statements keeps the bots off devices.
		MatchRequest recount = later;
		recount.players = 3;
		recount.seats = { seat_choice(2, "bot", "easy") };
		const MatchPlan three = planner.resolve(&plan.settings, recount, all_pads, random);
		require(three.ok && three.settings.player_count() == 3 &&
			three.settings.player_settings()[1].num == -1 && three.settings.player_settings()[2].num == -1 &&
			three.settings.screen_layout() == labrador::ScreenLayout::one_player,
			"A recount retains bots without devices and lays out the one human");

		// Refusals: a bot P1, a seat outside the roster, out-of-range values.
		const auto refused = [&](std::vector<MatchRequest::SeatChoice> choices, int players,
			const std::string& wanted, const std::string& why)
			{
				MatchRequest request;
				request.stage = "movement_course";
				request.players = players;
				request.seats = std::move(choices);
				const MatchPlan result = planner.resolve(nullptr, request, all_pads, random);
				require(!result.ok && result.error.find(wanted) != std::string::npos,
					why + ": got '" + result.error + "'");
			};
		MatchRequest::SeatChoice host;
		host.seat = 0;
		host.field = MatchRequest::SeatChoice::Field::bot;
		host.value = "hard";
		refused({ host }, 4, "seat 1 stays human", "A bot P1 is refused");
		refused({ seat_choice(3, "team", "a") }, 2, "is not in a 2-player match",
			"A seat outside the roster is refused");
		MatchRequest::SeatChoice boost = seat_choice(1, "boost", "2");
		boost.value = "3";
		refused({ boost }, 2, "boost must be 0, 1 or 2", "An out-of-range Boost is refused");
		boost.value = "-1";
		refused({ boost }, 2, "boost must be 0, 1 or 2", "A negative Boost is refused");
		MatchRequest::SeatChoice team = seat_choice(1, "team", "a");
		team.value = "none";
		refused({ team }, 2, "team must be a or b", "A team other than a or b is refused");
		MatchRequest::SeatChoice difficulty = seat_choice(1, "bot", "easy");
		difficulty.value = "expert";
		refused({ difficulty }, 2, "easy, normal or hard", "An unknown difficulty is refused");

		// A device on a bot seat cannot be planned: the claims skip bots, and
		// the planner checks the result before handing it out.
		for (const MatchPlanner::ConnectedPads& pads : { no_pads, all_pads })
		{
			for (int mask = 0; mask < 8; ++mask)
			{
				MatchRequest request;
				request.stage = "movement_course";
				request.players = 4;
				for (int seat = 1; seat < 4; ++seat)
				{
					if ((mask & (1 << (seat - 1))) != 0) { request.seats.push_back(seat_choice(seat, "bot", "normal")); }
				}
				const MatchPlan result = planner.resolve(nullptr, request, pads, random);
				require(result.ok, "Bot mask plan resolves: " + result.error);
				for (const SeatSetup& seat : result.settings.player_settings())
				{
					require(seat.controller == PlayerController::human || seat.num == -1,
						"A planned bot seat holds a device");
				}
				require(result.settings.screen_layout() == MatchPlanner::layout_for(result.settings.human_count()),
					"A planned layout is not the human layout");
			}
		}

		MatchRequest described;
		described.seats = { seat_choice(1, "bot", "hard"), seat_choice(2, "human", "") };
		require(!described.empty() && described.describe() == "seat 2 bot hard, seat 3 human",
			"A seat request describes itself, got '" + described.describe() + "'");
	}

	void check_roster_statements(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		const auto fails = [&](const std::string& line, const std::string& wanted)
			{
				const DebugStatus status = impl.execute(line);
				require(!status.ok && status.message.find(wanted) != std::string::npos,
					line + " is refused with '" + wanted + "', got '" + status.message + "'");
			};
		fails("seat 1 bot hard", "seat 1 stays human");
		fails("seat 5 team a", "seat must be 1-4");
		fails("seat 2 boost 3", "boost must be 0, 1 or 2");
		fails("seat 2 team c", "team must be a or b");
		fails("seat 2 bot expert", "easy, normal or hard");
		fails("seat 2 human now", "human takes no value");
		fails("seat 2 colour b", "unknown field");
		require(impl.request().seats.empty(), "Refused seat statements queue nothing");
		const DebugStatus stored = impl.execute("seat 3 bot easy");
		require(stored.ok && log_contains(impl.log(), "seat 3 bot easy: stored for 'stage <name>'",
			DebugSeverity::info) && impl.request().seats.size() == 1,
			"A seat statement in the menus is stored for a stage");
		require(impl.registry().check("seat 2 human").ok && impl.registry().check("seat 4 boost 0").ok &&
			!impl.registry().check("seat 2").ok, "Seat lines pass the script check by their arity");
		require(impl.execute("bot 2 paint").ok && impl.execute("bot off").ok,
			"bot <seat> <behaviour> keeps its meaning beside seat");
	}

	void check_roster_replay(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"roster_replay");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		CpuWorld world(game_directory, tuning, presentation);
		GameData data;
		data.set_stage_catalog(&world.catalog);
		DebugDirector director(&tuning, &presentation);
		director.attach(nullptr, &data);
		DebugDirectorImpl& impl = *director.impl();
		impl.log().set_mirror_to_debugger(false);
		impl.set_content_directories(scratch.source, scratch.staged);

		// A one-player match from the menus, then the statements a launch line
		// would carry, applied by the director's own rebuild.
		MatchRequest start;
		start.stage = "movement_course";
		start.players = 1;
		const MatchPlan first = MatchPlanner(&world.stages).resolve(nullptr, start, {}, [] { return 0u; });
		require(first.ok, "Cannot plan the roster replay's first match: " + first.error);
		CpuMatch match(world, director);
		match.settings = first.settings;
		director.attach_match(match);
		match.build();
		for (const char* line : { "seed 211", "bot all paint", "players 4", "seat 2 bot easy",
			"seat 2 boost 2", "seat 2 team b", "seat 3 bot hard", "seat 3 boost 1", "seat 3 team b",
			"seat 4 team a", "seat 1 boost 1", "weapon 3 roller", "stage close_quarters" })
		{
			const DebugStatus status = impl.execute(line);
			require(status.ok, std::string(line) + " is accepted: " + status.message);
		}
		const int builds = match.builds;
		for (int step = 0; step < 10 && match.builds == builds; ++step) { roster_tick(director, match); }
		require(match.builds == builds + 1 && impl.request().empty(), "The seat statements rebuild the match once");
		const MatchSetup planned = match.settings;
		const std::vector<SeatSetup> seats = planned.player_settings();
		require(planned.stage() == "close_quarters" && seats.size() == 4 &&
			seats[0].num == input_slots::keyboard_and_mouse && seats[0].team == PlayerTeam::a && seats[0].handicap == 1 &&
			seats[1].controller == PlayerController::bot && seats[1].difficulty == BotDifficulty::easy &&
			seats[1].team == PlayerTeam::b && seats[1].handicap == 2 &&
			seats[2].controller == PlayerController::bot && seats[2].difficulty == BotDifficulty::hard &&
			seats[2].team == PlayerTeam::b && seats[2].handicap == 1 && seats[2].weapon == WeaponType::roller &&
			seats[3].controller == PlayerController::human && seats[3].num == -1 && seats[3].team == PlayerTeam::a &&
			planned.screen_layout() == labrador::ScreenLayout::two_player,
			"The director built the planned roster: " + roster_text(planned));
		require(match.level->player_count() == 4, "The Level has every participant");
		require(impl.bot_for_seat(1, -1, true).empty() && impl.bot_for_seat(3, -1) == "paint",
			"bot all drives the idle dummy, never a shipping bot");
		require(impl.status_strip().find("BOTS 1x paint") != std::string::npos,
			"Only the dummy seat has a Debug behaviour, got '" + impl.status_strip() + "'");

		// Record from a fresh build, then play it back.
		const std::string name = "roster_" + std::to_string(_getpid());
		const std::filesystem::path path = std::filesystem::path(labrador::widen(
			directory_consts::replay_directory())) / labrador::path_from_utf8(name + ".json");
		const DebugStatus started = impl.execute("record " + name + " " + std::to_string(roster_length));
		require(started.ok && director.before_match_update(match, roster_step) && impl.recording(),
			"Cannot record the planned roster: " + started.message);
		require(recorded_roster(match.settings) == recorded_roster(planned), "Recording rebuilt a different roster");
		for (int step = 0; step < roster_length + 10 && impl.recording(); ++step) { roster_tick(director, match); }
		const ReplayFile file = ReplayFile::read(path);
		require(!impl.recording() && file.updates == roster_length && file.seed == 211u,
			"The planned roster's recording is complete and seeded");
		require(recorded_roster(file.tape.settings()) == recorded_roster(planned),
			"The recording reads back the planned roster: " + recorded_roster(file.tape.settings()));
		require(impl.execute("pause off").ok && impl.execute("play " + name).ok &&
			director.before_match_update(match, roster_step) && impl.playing(), "Cannot play the roster recording");
		require(recorded_roster(match.settings) == recorded_roster(planned), "Playback rebuilt a different roster");
		for (int step = 0; step < file.updates + 10 && !impl.replay_finished(); ++step) { roster_tick(director, match); }
		const std::optional<MatchFingerprint> print = impl.fingerprint();
		require(impl.replay_finished() && print && print->hash == file.final_print.hash &&
			MatchFingerprint::difference(file.final_print, *print).empty() &&
			log_contains(impl.log(), "replay " + name + ": MATCH (" + std::to_string(file.updates) + " updates",
				DebugSeverity::info), "The planned roster replays with matching checksums");
		require(impl.execute("play stop").ok, "Cannot stop the roster replay");
		director.detach_match(match);
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		std::printf("DBG17: seat statements planned a 2-bot Boost roster; %d updates recorded and replayed\n",
			roster_length);
	}
}
#endif
