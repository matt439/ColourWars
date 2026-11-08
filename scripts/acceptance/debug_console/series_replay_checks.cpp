#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "game/debug/replay/replay.h"
#include "game/directory_consts.h"
#include "scripts/acceptance/debug_console/cpu_level.h"
#include <process.h>

// DBG#2: a recording carried over a round boundary by the End menu's next
// round, written as format 9's later rounds, and played back round by round
// on a real Level. CpuMatch::build stands in for the End menu's next round:
// a build no debug request asked for.
namespace debug_console_acceptance
{
	namespace
	{
		constexpr float series_step = 1.0f / 60.0f;
		// Countdown, two seconds of play, the zoom-out and the overview, with room.
		constexpr int series_round_limit = 2000;

		void series_tick(DebugDirector& director, CpuMatch& match)
		{
			if (!director.before_match_update(match, series_step))
			{
				update_match(director, match, keyboard_step(director.impl()->level_updates()), series_step);
			}
		}

		int count_lines(const DebugLog& log, const std::string& text)
		{
			int count = 0;
			for (const DebugLogLine& line : log.lines())
			{
				count += line.text.find(text) != std::string::npos ? 1 : 0;
			}
			return count;
		}
	}

	void check_series_replay(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"series_replay");
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
		const DebugLog& log = impl.log();

		// Two seats, the second a paint bot, two-second rounds of a best of three.
		MatchRequest start;
		start.stage = "movement_course";
		start.players = 2;
		const MatchPlan first = MatchPlanner(&world.stages).resolve(nullptr, start, {}, [] { return 0u; });
		require(first.ok, "Cannot plan the series replay's match: " + first.error);
		require(impl.execute("bot all paint").ok, "bot all paint for the series");
		CpuMatch match(world, director);
		match.settings = first.settings;
		match.settings.set_duration(2.0f);
		match.settings.set_best_of(3);
		match.settings.set_variant(MatchVariant::endless_paint);
		match.drawn_seed = 101;
		director.attach_match(match);
		match.build();

		const std::string name = "series_" + std::to_string(_getpid());
		const std::filesystem::path path = std::filesystem::path(labrador::widen(
			directory_consts::replay_directory())) / labrador::path_from_utf8(name + ".json");
		const DebugStatus started = impl.execute("record " + name);
		require(started.ok && director.before_match_update(match, series_step) && impl.recording(),
			"Cannot record the series: " + started.message);

		// Round 1 ends: saved as one round, kept for the next round.
		for (int step = 0; step < series_round_limit && impl.recording(); ++step) { series_tick(director, match); }
		require(!impl.recording() && match.debug_level_finished() && impl.record_continues() &&
			count_lines(log, "record: saved") == 1 && count_lines(log, "; the round ended)") == 1 &&
			impl.status_strip().find("REC " + name + " next round") != std::string::npos,
			"The first round's end saves the recording and keeps it for the next round");
		const ReplayFile one = ReplayFile::read(path);
		require(one.round_count() == 1 && one.seed == 101u &&
			one.tape.settings().variant() == MatchVariant::endless_paint &&
			one.final_print.level_state == static_cast<int>(LevelState::finished) &&
			read_text(path).find("\"rounds\"") == std::string::npos,
			"After one round the file is a one-round file");

		// The End menu's next round, on the series' next stage, with an intro
		// the game would play: the recording carries on, from the first update.
		match.drawn_seed = 202;
		match.settings.set_stage("close_quarters");
		match.opening = OpeningMode::full;
		match.build();
		match.opening = OpeningMode::off;
		require(impl.recording() && impl.level_updates() == 0 && !match.level->intro_playing() &&
			log_contains(log, "record " + name + ": recording round 2", DebugSeverity::info),
			"The End menu's next round carries the recording on, without an intro");
		for (int step = 0; step < series_round_limit && impl.recording(); ++step) { series_tick(director, match); }
		require(!impl.recording() && match.debug_level_finished() && impl.record_continues() &&
			count_lines(log, " rounds, checksum ") == 1, "The second round's end saves both rounds");
		const ReplayFile both = ReplayFile::read(path);
		require(both.round_count() == 2 && both.later_rounds.size() == 1 &&
			both.round(0).seed == 101u && both.round(1).seed == 202u &&
			both.round(0).tape.settings().stage() == "movement_course" &&
			both.round(1).tape.settings().stage() == "close_quarters" &&
			both.round(0).tape.settings().variant() == MatchVariant::endless_paint &&
			both.round(1).tape.settings().variant() == MatchVariant::endless_paint &&
			both.round(1).stage_hash.has_value() && both.round(1).checkpoints.size() >= 5 &&
			both.round(1).final_print.level_state == static_cast<int>(LevelState::finished) &&
			both.round(1).build == both.build && both.round(1).dt_bits == both.dt_bits &&
			MatchFingerprint::difference(one.final_print, both.round(0).final_print).empty(),
			"The file holds both rounds, each with its own seed, stage and checks");
		const std::string bytes = read_text(path);
		require(bytes.find("\"version\": " + std::to_string(ReplayFile::format_version)) != std::string::npos &&
			bytes.find("\"rounds\": [") != std::string::npos, "The current format writes the later round");
		// A written file reads back to the same bytes.
		both.write(path);
		require(read_text(path) == bytes, "A two-round file round-trips byte for byte");

		// A debug rebuild is not the next round: it ends the recording as saved.
		require(impl.execute("restart").ok, "restart between rounds");
		const int builds = match.builds;
		series_tick(director, match);
		require(match.builds == builds + 1 && !impl.recording() && !impl.record_continues() &&
			log_contains(log, "record " + name + ": ends with 2 rounds, as saved", DebugSeverity::info) &&
			read_text(path) == bytes, "A debug restart ends the recording without touching its file");

		// Play: each round is built from its own seed and stage and compared.
		const DebugStatus played = impl.execute("play " + name);
		// A command's success message goes to the log, not the returned status.
		require(played.ok && count_lines(log, " updates in round 1 of 2: rebuilding the match") == 1 &&
			director.before_match_update(match, series_step) && impl.playing() &&
			match.settings.stage() == "movement_course",
			"Cannot play the series: " + played.message);
		const int play_builds = match.builds;
		for (int step = 0; step < series_round_limit * 2 && !impl.replay_finished(); ++step)
		{
			series_tick(director, match);
		}
		const std::optional<MatchFingerprint> print = impl.fingerprint();
		require(impl.replay_finished() && match.builds == play_builds + 1 &&
			match.settings.stage() == "close_quarters" && print &&
			MatchFingerprint::difference(both.round(1).final_print, *print).empty() &&
			log_contains(log, "replay " + name + " round 1/2: MATCH (", DebugSeverity::info) &&
			log_contains(log, "replay " + name + " round 2/2: MATCH (", DebugSeverity::info) &&
			log_contains(log, "replay " + name + ": MATCH in all 2 rounds", DebugSeverity::info),
			"Both rounds replay with matching checksums");
		require(impl.execute("play stop").ok && !impl.playing(), "Cannot stop the series replay");
		director.detach_match(match);
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		std::printf("DBG#2: a recording carried across a round boundary (%d + %d updates, two stages) "
			"replays round by round\n", both.round(0).updates, both.round(1).updates);
	}
}
#endif
