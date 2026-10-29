#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot.h"
#include "game/debug/debug_script.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/debug_wait.h"
#include "game/objects/level_loaded_info.h"
#include "game/source_paths.h"
#include "engine/render/text_encoding.h"
#include <functional>
#include <iostream>
#include <map>
#include <optional>

// Scripts: parsing, the runner through the director's real hooks, the shipped
// scripts followed and linted, and wait failures.
namespace debug_console_acceptance
{
	void check_script_parse()
	{
		ParsedScript parsed = parse_script("x.cfg",
			"\xEF\xBB\xBF// summary\r\necho \"a // b\" // comment\r\n\r\n   \r\nstage x; players 2\r\n", {});
		require(parsed.ok && parsed.statements.size() == 3, "BOM, CRLF, blank lines and comments: " +
			parsed.error);
		require(parsed.statements[0].text == "echo \"a // b\"" && parsed.statements[0].line == 2,
			"// inside quotes is text, outside it is a comment, got '" + parsed.statements[0].text + "'");
		require(parsed.statements[1].text == "stage x" && parsed.statements[2].text == "players 2" &&
			parsed.statements[1].line == 5 && parsed.statements[2].line == 5,
			"a; b is two statements on one line");
		require(parsed.statements[1].where() == "x.cfg:5", "where() is file:line");

		parsed = parse_script("x.cfg", "feedback $1\necho $$1 costs $2\n", { "on", "t040" });
		require(parsed.ok && parsed.statements[0].text == "feedback on" &&
			parsed.statements[1].text == "echo $1 costs t040", "$n is substituted and $$ is a dollar");
		parsed = parse_script("x.cfg", "echo a\necho b\nscreenshot $1-$2\n", { "only" });
		require(!parsed.ok && parsed.error.find("x.cfg:3") != std::string::npos &&
			parsed.error.find("$2 has no argument") != std::string::npos,
			"A missing argument fails at its line: " + parsed.error);
		parsed = parse_script("x.cfg", "feedback $1\n", { "a;quit" });
		require(!parsed.ok && parsed.error.find("argument $1") != std::string::npos,
			"An argument cannot add a statement: " + parsed.error);
		parsed = parse_script("x.cfg", "echo ok\necho caf\xC3\xA9\n", {});
		require(!parsed.ok && parsed.error.find("x.cfg:2: only printable ASCII") != std::string::npos,
			"Non-ASCII fails with file:line: " + parsed.error);
		parsed = parse_script("x.cfg", "echo \"open\n", {});
		require(!parsed.ok && parsed.error.find("x.cfg:1: unterminated quote") != std::string::npos,
			"An open quote fails with file:line: " + parsed.error);
		require(!parse_script("x.cfg", std::string(debug_script::max_bytes + 1, ' '), {}).ok &&
			parse_script("x.cfg", std::string(debug_script::max_bytes, ' '), {}).ok,
			"A script is at most 64 KiB");

		std::string file;
		require(debug_script::file_name("review_feedback_ab", file) && file == "review_feedback_ab.cfg" &&
			debug_script::file_name("a-b.cfg", file) && file == "a-b.cfg" &&
			!debug_script::file_name("../x", file) && !debug_script::file_name("C:/x", file) &&
			!debug_script::file_name("", file), "Script names are names, never paths");
	}

	// The runner, through the director's real hooks, against the fake match
	// and a fake page, with scratch script and user folders.
	void check_script_runner(GameTuning& tuning, Presentation& presentation)
	{
		ScratchFolder scratch(L"scripts");
		const std::filesystem::path scripts = scratch.root / L"scripts";
		const std::filesystem::path user = scratch.root / L"user";
		std::filesystem::create_directories(scripts);
		std::filesystem::create_directories(user);
		write_text(scripts / L"hello.cfg", "// says hello\necho hello-from-script\n");
		write_text(scripts / L"waits.cfg", "echo w-start-$1\nwait 10\necho w-done-$1\n");
		write_text(scripts / L"seconds.cfg", "wait 0.5s\necho w-half\n");
		write_text(scripts / L"matchwait.cfg", "stage x\nwait match\necho w-matched\n");
		write_text(scripts / L"frames.cfg", "wait frames 3\necho w-frames\n");
		write_text(scripts / L"err.cfg", "echo e1\necho e2\necho e3\nbogus_command\necho never-reached\n");
		write_text(scripts / L"rec.cfg", "exec rec\n");
		for (int i = 1; i <= 9; ++i)
		{
			write_text(scripts / (L"deep" + std::to_wstring(i) + L".cfg"), i < 9 ?
				"exec deep" + std::to_string(i + 1) + "\n" : std::string("echo too-deep\n"));
			write_text(scripts / (L"nest" + std::to_wstring(i) + L".cfg"), i < 8 ?
				"exec nest" + std::to_string(i + 1) + "\n" : std::string("echo nested-eight\n"));
		}
		write_text(scripts / L"shot.cfg", "screenshot barrier\necho after-shot\n");
		write_text(scripts / L"args.cfg", "echo args $1 $2\n");
		write_text(user / L"mine.cfg", "echo from-the-profile\n");

		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		DebugDirector& director = test.director;
		impl.set_script_directories({ scripts, user });
		const DebugLog& log = impl.log();
		const float dt = 1.0f / 60.0f;

		// DBG12: discovery and execution share private directories. A separate
		// director keeps these scripts, logs and frames out of the runner below.
		{
			const std::filesystem::path caller_directory = std::filesystem::current_path();
			const auto caller_profile = []
			{
				const DWORD needed = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
				if (needed == 0) { return std::wstring(); }
				std::vector<wchar_t> value(needed);
				const DWORD written = GetEnvironmentVariableW(L"LOCALAPPDATA", value.data(), needed);
				require(written > 0 && written < needed, "DBG12 could not observe caller profile state");
				return std::wstring(value.data(), written);
			};
			const std::wstring profile_before = caller_profile();
			const std::filesystem::path discovery = scratch.root / L"discovery";
			const std::filesystem::path first = discovery / L"first";
			const std::filesystem::path second = discovery / L"second";
			const std::filesystem::path profile = discovery / L"profile";
			for (const auto& directory : { first, second, profile, first / L"folderonly.cfg", first / L"fallback.cfg" })
			{
				std::filesystem::create_directories(directory);
			}
			const std::vector<std::pair<std::filesystem::path, std::string>> files{
				{ first / L"Upper.CFG", "// uppercase first\necho DBG12_UPPER_FIRST\n" },
				{ first / L"MiXeD.CfG", "// mixed first\necho DBG12_MIXED_FIRST\n" },
				{ first / L"lower.cfg", "// lowercase first\necho DBG12_LOWER_FIRST\n" },
				{ first / L"shared.CFG", "// first folder wins\necho DBG12_SHARED_FIRST\n" },
				{ second / L"UserUpper.CFG", "// uppercase second\necho DBG12_UPPER_SECOND\n" },
				{ second / L"UserMixed.cFg", "// mixed second\necho DBG12_MIXED_SECOND\n" },
				{ second / L"user_lower.cfg", "// lowercase second\necho DBG12_LOWER_SECOND\n" },
				{ second / L"shared.cfg", "// second folder loses\necho DBG12_SHARED_SECOND\n" },
				{ second / L"fallback.CfG", "// later regular file\necho DBG12_FALLBACK_SECOND\n" },
				{ first / L"folderonly.cfg" / L"nested.cfg", "echo DBG12_NESTED_MUST_NOT_RUN\n" },
				{ first / L"fallback.cfg" / L"caller.keep", "unchanged directory decoy bytes\n" },
				{ first / L"bad name.cfg", "echo DBG12_INVALID_SPACE_MUST_NOT_RUN\n" },
				{ first / L"bad.name.cfg", "echo DBG12_INVALID_DOT_MUST_NOT_RUN\n" },
				{ first / (std::wstring(65, L'x') + L".cfg"), "echo DBG12_LONG_NAME_MUST_NOT_RUN\n" },
				{ first / L"wrong.txt", "echo DBG12_WRONG_EXTENSION_MUST_NOT_RUN\n" },
				{ second / L"almost.cfg.bak", "echo DBG12_SUFFIX_MUST_NOT_RUN\n" },
				{ profile / L"caller.keep", "unchanged isolated profile bytes\n" }
			};
			for (const auto& file : files) { write_text(file.first, file.second); }
			const auto snapshot = [&discovery]
			{
				std::map<std::wstring, std::string> tree;
				for (const auto& entry : std::filesystem::recursive_directory_iterator(discovery))
				{
					const std::wstring name = entry.path().lexically_relative(discovery).native();
					if (entry.is_directory()) { tree.emplace(name + L"/", "directory"); }
					else
					{
						require(entry.is_regular_file(), "DBG12 private tree contains an unexpected entry type");
						tree.emplace(name, read_text(entry.path()));
					}
				}
				return tree;
			};
			const auto tree_before = snapshot();
			TestDirector discovery_test(tuning, presentation);
			DebugDirectorImpl& discovery_impl = discovery_test.impl;
			require(!discovery_impl.user_directory().has_value(),
				"DBG12 application-free constructor unexpectedly selected a player profile");
			discovery_impl.set_user_directory(profile);
			discovery_impl.set_script_directories({ first, second });
			DebugLog& discovery_log = discovery_impl.log();
			const auto info_lines = [&discovery_log]
			{
				std::vector<std::string> lines;
				for (const auto& line : discovery_log.lines())
				{
					if (line.severity == DebugSeverity::info) { lines.push_back(line.text); }
				}
				return lines;
			};
			discovery_log.clear();
			discovery_test.director.run_launch_arguments({ L"+exec", L"Upper" });
			const auto upper_output = info_lines();
			require(std::count(upper_output.begin(), upper_output.end(), "DBG12_UPPER_FIRST") == 1 &&
				!discovery_impl.scripts().running(),
				"DBG12 private uppercase-extension script was not executable by its stem");
			// Deliberately first new regression after proving actual execution.
			const DebugCompletion upper = discovery_impl.registry().complete("exec Upp");
			require(upper.line == "exec Upper " && upper.candidates.empty(),
				"DBG12 completion omitted an executable uppercase-extension script");

			const DebugCompletion all = discovery_impl.registry().complete("exec ");
			require(all.line == "exec " && all.candidates == std::vector<std::string>{
				"MiXeD", "Upper", "UserMixed", "UserUpper", "fallback", "lower", "shared", "user_lower" },
				"DBG12 discovery changed sorted unique names, stem spelling or candidate admission");
			require(discovery_impl.registry().complete("exec MiX").line == "exec MiXeD " &&
				discovery_impl.registry().complete("exec UserM").line == "exec UserMixed " &&
				discovery_impl.registry().complete("exec UserU").line == "exec UserUpper " &&
				discovery_impl.registry().complete("exec low").line == "exec lower " &&
				discovery_impl.registry().complete("exec user_l").line == "exec user_lower " &&
				discovery_impl.registry().complete("exec fal").line == "exec fallback ",
				"DBG12 mixed/lowercase extensions or later-folder regular files lost completion");
			for (const char* line : { "exec folder", "exec nested", "exec wrong", "exec almost", "exec bad" })
			{
				const DebugCompletion none = discovery_impl.registry().complete(line);
				require(none.line == line && none.candidates.empty(),
					"DBG12 completion advertised a directory, nested file, wrong extension or invalid stem");
			}
			discovery_log.clear();
			require(discovery_impl.execute("exec.list").ok, "DBG12 private script listing failed");
			const auto listed = info_lines();
			const std::vector<std::string> expected_listing{
				"  MiXeD  - mixed first", "  Upper  - uppercase first", "  UserMixed  - mixed second",
				"  UserUpper  - uppercase second", "  fallback  - later regular file", "  lower  - lowercase first",
				"  shared  - first folder wins", "  user_lower  - lowercase second"
			};
			require(listed.size() == 9 && std::equal(expected_listing.begin(), expected_listing.end(), listed.begin()) &&
				listed.back().starts_with("8 script(s) in "),
				"DBG12 listing lost exact summaries, regular-file filtering, sorted deduplication or folder precedence");
			discovery_log.clear();
			discovery_test.director.run_launch_arguments({ L"+exec", L"MiXeD", L"+exec", L"lower",
				L"+exec", L"UserUpper", L"+exec", L"UserMixed", L"+exec", L"user_lower",
				L"+exec", L"shared", L"+exec", L"fallback", L"+exec", L"Upper.cfg" });
			const auto executed = info_lines();
			for (const char* output : { "DBG12_MIXED_FIRST", "DBG12_LOWER_FIRST", "DBG12_UPPER_SECOND",
				"DBG12_MIXED_SECOND", "DBG12_LOWER_SECOND", "DBG12_SHARED_FIRST", "DBG12_FALLBACK_SECOND", "DBG12_UPPER_FIRST" })
			{
				require(std::count(executed.begin(), executed.end(), output) == 1,
					"DBG12 discovered script did not execute exactly once with existing folder precedence");
			}
			require(std::count(executed.begin(), executed.end(), "DBG12_SHARED_SECOND") == 0 &&
				!discovery_impl.scripts().running(), "DBG12 execution selected the later duplicate or retained a script frame");
			discovery_log.clear();
			require(!discovery_impl.execute("exec Upper.CFG").ok && !discovery_impl.execute("exec UserMixed.cFg").ok &&
				!discovery_impl.execute("exec folderonly").ok && !discovery_impl.execute("exec nested").ok &&
				!discovery_impl.execute("exec wrong").ok && !discovery_impl.execute("exec almost").ok &&
				!discovery_impl.execute("exec \"bad name\"").ok && !discovery_impl.execute("exec bad.name").ok &&
				!discovery_impl.execute("exec " + std::string(65, 'x')).ok && info_lines().empty() &&
				!discovery_impl.scripts().running(),
				"DBG12 changed explicit suffix/name refusals or executed an excluded candidate");
			require(snapshot() == tree_before && std::filesystem::current_path() == caller_directory &&
				caller_profile() == profile_before,
				"DBG12 discovery/execution changed private files, directory entries or caller path/profile state");
			for (const auto& file : files)
			{
				require(read_text(file.first) == file.second, "DBG12 changed a literal script, decoy or profile input");
			}
		}

		require(!impl.execute("exec nope").ok && any_line(log, "exec: no nope.cfg in"),
			"exec of a missing script fails and says where it looked");
		require(!impl.execute("exec ../x").ok && any_line(log, "is not a script name"),
			"exec takes a name, never a path");
		require(!impl.execute("wait 10").ok &&
			any_line(log, "wait: only in a script (exec, autoexec.cfg or launch arguments)"),
			"wait outside a script fails");
		require(impl.execute("exec.list").ok && any_line(log, "  hello  - says hello") &&
			any_line(log, "  mine"), "exec.list lists both folders with their summaries");

		// Launch arguments are a script frame: exec runs to completion at once.
		director.run_launch_arguments({ L"+exec", L"hello", L"+exec", L"args", L"one", L"two",
			L"+exec", L"mine" });
		require(any_line(log, "hello-from-script") && any_line(log, "script hello.cfg done") &&
			any_line(log, "args one two") && any_line(log, "from-the-profile") &&
			any_line(log, "launch: exec hello") && any_line(log, "hello.cfg:2] echo hello-from-script"),
			"A launch exec runs its script, echoing file:line, then the next launch statement");

		// An error stops the script at its line; the launch statements go on.
		director.run_launch_arguments({ L"+exec", L"err", L"+echo", L"after-err" });
		// Not "e5": the console's banner prints the build's commit hash.
		require(any_line(log, "e3") && !any_line(log, "never-reached") &&
			any_line(log, "script err.cfg:4 stopped: unknown command 'bogus_command'") &&
			any_line(log, "after-err"), "An error stops the script with file:line; launch continues");

		// Nesting: eight deep runs, nine is refused, and a script cannot exec itself.
		director.run_launch_arguments({ L"+exec", L"nest1" });
		require(any_line(log, "nested-eight"), "Eight nested scripts run");
		director.run_launch_arguments({ L"+exec", L"deep1" });
		require(!any_line(log, "too-deep") && any_line(log, "would nest deeper than 8 scripts"),
			"A ninth level is refused");
		director.run_launch_arguments({ L"+exec", L"rec" });
		require(any_line(log, "exec: rec.cfg is already running"), "Recursion is refused");
		require(!impl.scripts().running(), "Every frame has finished");

		// Waits, in a match.
		FakeMatch match(director);
		match.settings = planned(1);
		director.attach_match(match);
		const auto hook = [&] { director.before_match_update(match, dt); };
		const std::vector<PlayerInputData> none(input_slots::count);
		const auto update = [&] { director.update_match(match, none, dt); };

		require(impl.execute("exec waits a").ok, "exec typed at the console is accepted");
		require(!impl.execute("exec hello").ok && any_line(log, "exec: waits.cfg is running ('exec.stop' first)"),
			"A second exec at the console is refused while one runs");
		hook();
		require(any_line(log, "w-start-a") && !any_line(log, "w-done-a"), "The script runs up to its wait");
		require(impl.status_strip().find("SCRIPT waits.cfg:2 wait 10 updates (10 left)") != std::string::npos,
			"The strip names the script and its wait, got '" + impl.status_strip() + "'");
		match.reset_counts();
		for (int i = 0; i < 9; ++i) { update(); hook(); }
		require(match.updates == 9 && !any_line(log, "w-done-a"), "Nine updates are not ten");
		update(); hook();
		require(match.updates == 10 && any_line(log, "w-done-a"), "wait 10 runs the next line after ten updates");

		// Timescale 4: the third step stops after two updates.
		require(impl.execute("timescale 4").ok && impl.execute("exec waits b").ok, "exec again at x4");
		hook();
		match.reset_counts();
		update(); hook(); update(); hook();
		require(match.updates == 8 && !any_line(log, "w-done-b"), "Two steps at x4 are eight updates");
		update();
		require(match.updates == 10, "The third step stops at the tenth update, got " +
			std::to_string(match.updates));
		hook();
		require(any_line(log, "w-done-b"), "and the next line runs before any other update");
		impl.execute("timescale 1");

		// Paused: no progress; `step 10` completes the wait.
		require(impl.execute("pause on").ok && impl.execute("exec waits c").ok, "exec while paused");
		hook();
		match.reset_counts();
		for (int i = 0; i < 5; ++i) { update(); hook(); }
		require(match.updates == 0 && !any_line(log, "w-done-c"), "A paused clock makes no progress");
		require(impl.status_strip().find("the clock is paused") != std::string::npos,
			"The strip says why the script is stuck");
		require(impl.execute("step 10").ok, "step 10");
		for (int i = 0; i < 10; ++i) { update(); hook(); }
		require(match.updates == 10 && any_line(log, "w-done-c"), "step 10 completes wait 10");
		impl.execute("pause off");

		// Seconds are updates at the engine's step.
		require(impl.execute("exec seconds").ok, "exec seconds");
		hook();
		require(impl.status_strip().find("wait 30 updates") != std::string::npos,
			"wait 0.5s is 30 updates at 1/60, got '" + impl.status_strip() + "'");
		for (int i = 0; i < 30; ++i) { update(); hook(); }
		require(any_line(log, "w-half"), "and runs on after them");

		// wait match: through the rebuild, then the countdown.
		match.countdown_over = false;
		const int rebuilds = match.rebuilds;
		require(impl.execute("exec matchwait").ok, "exec matchwait");
		hook();
		require(match.rebuilds == rebuilds + 1 && !any_line(log, "w-matched"),
			"The queued stage rebuilds in the same hook; the wait holds for the countdown");
		update(); hook();
		require(!any_line(log, "w-matched"), "Still counting down");
		match.countdown_over = true;
		update(); hook();
		require(any_line(log, "w-matched"), "wait match ends with the countdown");

		// A wait whose match ends stops the script.
		require(impl.execute("exec waits d").ok, "exec waits once more");
		hook();
		director.detach_match(match);
		FakePage page;
		director.before_page_update(page, dt);
		require(any_line(log, "script waits.cfg:2: the match ended while waiting; stopped") &&
			!impl.scripts().running(), "A wait for updates fails when the match ends");

		// Frames count in the menus too.
		require(impl.execute("exec frames").ok, "exec frames");
		for (int i = 0; i < 3; ++i) { director.before_page_update(page, dt); }
		require(!any_line(log, "w-frames"), "Three frames after the wait was set are not over yet");
		director.before_page_update(page, dt);
		require(any_line(log, "w-frames"), "wait frames 3 ends on the third hook after it");

		// exec.stop clears the stack.
		require(impl.execute("exec matchwait").ok, "exec matchwait in the menus");
		director.before_page_update(page, dt);
		require(impl.scripts().running() && impl.execute("exec.stop").ok && !impl.scripts().running() &&
			any_line(log, "script stopped at matchwait.cfg:2 (exec.stop)"), "exec.stop stops it");
		impl.drop_request("test");

		// A pending screenshot holds the next line (no window here, so it never
		// clears; exec.stop ends it).
		require(impl.execute("exec shot").ok, "exec shot");
		director.before_page_update(page, dt);
		director.before_page_update(page, dt);
		require(impl.pending_screenshot().has_value() && !any_line(log, "after-shot") &&
			impl.status_strip().find("SCRIPT shot.cfg:1 screenshot pending") != std::string::npos,
			"A screenshot is a barrier: the next line waits for the shot");
		impl.execute("exec.stop");

		// autoexec.cfg runs before the launch statements, and only from the
		// user folder this director was given.
		write_text(user / L"autoexec.cfg", "echo autoexec-ran\n");
		TestDirector second(tuning, presentation);
		second.impl.set_user_directory(user);
		second.director.run_launch_arguments({ L"+echo", L"launch-ran" });
		const std::size_t autoexec = log_index(second.impl.log(), "autoexec-ran");
		const std::size_t launched = log_index(second.impl.log(), "launch-ran");
		require(autoexec != std::string::npos && launched != std::string::npos && autoexec < launched,
			"autoexec.cfg runs first");
		TestDirector third(tuning, presentation);
		require(!third.impl.user_directory().has_value(), "No application and no override: no user folder");
		third.director.run_launch_arguments({ L"+echo", L"x" });
		require(!any_line(third.impl.log(), "autoexec-ran"), "and no autoexec");
	}

	// Every shipped script, followed from the top-level ones through their
	// exec calls with the arguments they pass: every statement names an entry
	// with an accepted word count, and every stage, bot and station
	// exists. So names in scripts cannot drift.
	void check_shipped_scripts(GameTuning& tuning, Presentation& presentation,
		const std::filesystem::path& game_directory)
	{
		const std::filesystem::path folder(labrador::widen(source_paths::scripts_directory));
		TestDirector test(tuning, presentation);
		const DebugRegistry& registry = test.impl.registry();
		LevelLoadedInfo course((game_directory / L"levels" / L"movement_course.json").string().c_str());
		std::vector<std::string> stations;
		for (const LevelStation& station : course.stations()) { stations.push_back(station.name); }

		std::vector<std::string> names;
		for (const auto& entry : std::filesystem::directory_iterator(folder))
		{
			if (entry.path().extension() == L".cfg") { names.push_back(entry.path().stem().string()); }
		}
		require(names.size() >= 5, "scripts/debug holds the shipped scripts");
		std::map<std::string, int> visited;
		int statements = 0;
		std::function<void(const std::string&, const std::vector<std::string>&, int)> lint =
			[&](const std::string& name, const std::vector<std::string>& args, int depth)
			{
				require(depth <= 8, name + " nests too deep");
				const std::filesystem::path path = folder / (labrador::widen(name) + L".cfg");
				require(std::filesystem::is_regular_file(path), "exec target " + name + ".cfg exists");
				++visited[name];
				const ParsedScript parsed = parse_script(name + ".cfg", read_text(path), args);
				require(parsed.ok, parsed.error);
				for (const DebugScriptStatement& statement : parsed.statements)
				{
					++statements;
					const DebugStatus checked = registry.check(statement.text);
					require(checked.ok, statement.where() + ": " + checked.message);
					const DebugTokens tokens = tokenize(statement.text);
					const std::vector<std::string>& w = tokens.words;
					const auto word = [&w](std::size_t i) { return i < w.size() ? w[i] : std::string(); };
					if (w[0] == "exec")
					{
						lint(word(1), std::vector<std::string>(w.begin() + 2, w.end()), depth + 1);
					}
					else if (w[0] == "stage")
					{
						require(std::filesystem::is_regular_file(game_directory / L"levels" /
							(labrador::widen(word(1)) + L".json")), statement.where() + ": no stage " + word(1));
					}
					else if (w[0] == "goto")
					{
						require(std::find(stations.begin(), stations.end(), word(1)) != stations.end(),
							statement.where() + ": no Movement Course station " + word(1));
					}
					else if (w[0] == "bot" && w.size() == 3)
					{
						require(word(2) == "off" || Bot::is_behaviour(word(2)),
							statement.where() + ": no bot " + word(2));
					}
					else if (w[0] == "players" && w.size() == 3)
					{
						require(Bot::is_behaviour(word(2)), statement.where() + ": no bot " + word(2));
					}
				}
			};
		for (const std::string& name : names)
		{
			const std::string text = read_text(folder / (labrador::widen(name) + L".cfg"));
			require(text.rfind("// ", 0) == 0, name + ".cfg starts with a // summary line");
			if (text.find('$') == std::string::npos)
			{
				lint(name, {}, 0);
			}
		}
		for (const std::string& name : names)
		{
			require(visited.count(name) != 0, name + ".cfg is reached by a top-level script");
		}
		std::cout << "NOTE shipped scripts: " << names.size() << " files, " << statements <<
			" statements followed\n";
	}

	void check_wait_failures()
	{
		DebugWaitContext context;
		context.in_match = true;
		context.updates = 100;
		const UpdatesWait updates(400, 300);
		require(!updates.failed(context), "A wait in a running round has not failed");
		context.round_over = true;
		const std::optional<std::string> over = updates.failed(context);
		require(over && over->find("the round ended while waiting (300 updates left)") == 0,
			"A wait <n> past the end of the round fails, got '" + over.value_or("") + "'");
		context.build_pending = true;
		require(!updates.failed(context), "Not while a rebuild is queued (restart after results)");
		context.build_pending = false;
		context.updates = 400;
		require(!updates.failed(context), "Not when it was satisfied by the round's last update");

		DebugWaitContext armed;
		armed.recording = true;
		armed.record_armed = true;
		const RecordWait record;
		const std::optional<std::string> nothing = record.failed(armed);
		require(nothing && nothing->find("no match is queued to start the armed recording") == 0,
			"wait record with nothing to start it fails");
		armed.build_pending = true;
		require(!record.failed(armed), "wait record waits for the queued build");
		armed.in_match = true;
		armed.record_armed = false;
		armed.build_pending = false;
		require(!record.failed(armed) && !record.satisfied(armed), "A running recording waits");
	}
}
#endif
