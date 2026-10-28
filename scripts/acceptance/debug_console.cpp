#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"
#include "game/debug/debug_tokenizer.h"
#include "game/input_slots.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include <process.h>

// Session-only developer policy must not add conditional members to the
// service that GameData borrows in both configurations.
static_assert(sizeof(DebugDirector) == sizeof(void*));

#if COLOURWARS_DEV_TOOLS
#include "game/source_paths.h"
#include "engine/render/text_encoding.h"
#include <cstdlib>
#else
#include <io.h>
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
// The checks live in scripts/acceptance/debug_console/; this file is the
// driver and the Release-only checks.
namespace debug_console_acceptance
{
#if COLOURWARS_DEV_TOOLS
	void check_bot_status_labels(GameTuning& tuning, Presentation& presentation);
#endif
	namespace
	{
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
		// Developer bots, record/play and live tuning. SeatInputFilter is also
		// used by the shipping MatchBotDriver and therefore exists in Release.
		".?AVBot@@", ".?AVPaintBot@@", ".?AVBotDriver@@", ".?AVReplayRecorder@@",
		".?AVReplayPlayer@@", ".?AVTuneTarget@@", ".?AVTuningTarget@@",
		".?AVDebugBackgroundInput@@",
		// Phase 4: script waits.
		".?AVDebugWait@@", ".?AVUpdatesWait@@", ".?AVMatchWait@@",
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
		// The observer is non-polymorphic, so an RTTI absence assertion would
		// say nothing about it. The command/diagnostic token is Debug-only,
		// while the unconditional size assertion above checks the shared API.
		require((text.find("script.background") != std::string::npos) == dev_tools::enabled,
			"script.background command/observer diagnostic must be Debug-only");
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
	}
}

int main(int argc, char** argv)
{
	using namespace debug_console_acceptance;

	try
	{
		require(argc == 2 || (argc == 3 && std::string(argv[2]) == "--background-only"),
			"Usage: debug_console.exe <game directory> [--background-only]");
		const bool background_only = argc == 3;
		const std::filesystem::path game_directory = argv[1];
		check_executable(game_directory);
		check_loaded_stage_semantics();
		check_launch_detection();

		GameTuning tuning((game_directory / "tuning.json").string().c_str());
		Presentation presentation((game_directory / "presentation.json").string().c_str());

#if COLOURWARS_DEV_TOOLS
		// The focused path keeps the existing clock, hooks, scripts and replay
		// regressions that background updates can affect. With no flag, every
		// check below still runs in its original order, plus the new policy case.
		check_tokenizer();
		check_launch_arguments();
		check_registry();
		check_clock();
		check_planner();
		if (!background_only) { check_text_field(); }
		check_director(tuning, presentation);
		check_match_hooks(tuning, presentation);
		check_bot_status_labels(tuning, presentation);
		check_background_policy(game_directory, tuning, presentation);
		check_chord();
		check_input_filter(tuning, presentation);
		if (!background_only)
		{
			check_registry_menu_traits(tuning, presentation);
			check_menu_model(tuning, presentation);
		}
		check_unwinding(tuning, presentation);
		if (!background_only)
		{
			check_overlays_and_cheats(tuning, presentation);
			check_goto(tuning, presentation);
			check_screenshot_file(tuning, presentation);
			check_stations(game_directory, tuning);
		}

		// Phase 4. Scripts, binds and history use scratch folders; the
		// developer's own profile is never read or written.
		check_script_parse();
		check_script_runner(tuning, presentation);
		if (!background_only)
		{
			check_shipped_scripts(tuning, presentation, game_directory);
			check_binds(tuning, presentation);
			check_history(tuning, presentation);
		}
		check_live_console_input(tuning, presentation);
		if (!background_only) { check_perf(); }
		check_wait_failures();
		if (!background_only) { check_polish(tuning, presentation); }

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
			if (!background_only)
			{
				check_json_spans(game_directory);
				check_tune_scope(game_directory);
				const int consumed = check_tune_consumed(game_directory);
				std::cout << "NOTE consumed-leaf check: " << consumed << " numbers and booleans, each read "
					"by the game\n";
				check_tune_commands(game_directory);
				check_tune_completion_and_menu(game_directory);
			}
			check_bots(tuning, presentation);
			check_seeded_planner(tuning, presentation);
			check_replay_codec();
			replay_case = check_replay_cpu_match(game_directory);
			if (!background_only)
			{
				check_opening(game_directory);
				check_station_labels(game_directory);
			}
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
		if (background_only)
		{
			std::cout << "PASS debug console background-only (Debug): background CPU policy and "
				"queued-edge/OFF-batch regressions; existing tokenizer, registry, clock, planner, director/match "
				"hooks, chord/input filtering, unwinding/live-console blanking, script parser/runner/waits, "
				"bots/seeded planner and replay codec/CPU match (" << replay_case << "); isolated profile and "
				"source content unchanged; Debug-only command token and shared director layout. "
				"No native focus/audio acceptance.\n";
			return 0;
		}

		std::cout << "PASS debug console (Debug): tokenizer, launch arguments, registry, "
			"completion, clock, planner, text field, director, match hooks, chord, input "
			"filter, menu traits, menu model, unwinding, overlays and cheats, goto, "
			"screenshot file, stations; phase 3: JSON spans, tune scopes, consumed leaves, "
			"tune/diff/reset/save (CRLF and LF, conflicts), tune completion and menu tree, "
			"bots, seeded planner, replay codec and file, record/play on a " << replay_case <<
			" (MATCH back to back, at timescale 1 and 4, another size and stepped one update "
			"at a time; DIVERGED with other tuning; refused at another dt; disarmed by leaving; "
			"started from the menus); phase 4: script parsing, the runner (waits in updates, "
			"seconds, frames and for the match, exact at x4 and when paused, errors with "
			"file:line, nesting, exec.stop, autoexec first, the screenshot barrier), the "
			"shipped scripts followed and linted, binds (names, defaults, modifiers, reserved "
			"keys, binds.cfg round trip), history.txt, live-console input blanking, the perf "
			"ring, panel placement, staged notes, seat labels, stale tune folders, scripts "
			"under a replay; RTTI present.\n";
#else
		DebugDirector director(&tuning, &presentation);
		require(!director.enabled() && director.impl() == nullptr,
			"Release has no director implementation");
		require(!DebugDirector::inert().enabled(), "The inert director is inert");
		require(!director.background_scripts() && !director.background_updates_allowed() &&
			!DebugDirector::inert().background_scripts() && !DebugDirector::inert().background_updates_allowed(),
			"Release and inert directors never opt into background work");
		director.sync_background_audio(false);
		director.sync_background_audio(true);
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
		director.sync_background_audio(false);
		director.sync_background_audio(true);
		director.before_level_build(match);
		director.after_level_build(match);
		require(!director.before_match_update(match, 1.0f / 60.0f), "Release never takes an update");
		director.update_match(match, inputs, 1.0f / 60.0f);
		director.detach_match(match);
		require(match.overlay_sets == 0 && match.player_text_sets == 0 && match.cheat_sets == 0 &&
			match.rebuilds == 0 && match.adopts == 0 && match.placed.empty() &&
			match.respawned.empty() && match.updates == 1,
			"Release pushes no overlay, player text or cheat into a match and runs one update");
		require(match.background_changes == 0 && match.audio_syncs == 0 &&
			!director.background_scripts() && !director.background_updates_allowed(),
			"Release background policy/audio hooks remain inert for an attached target");
		require(match.last_filter == nullptr && match.filtered_updates == 0,
			"Release director adds no developer bot, recording or replay filter");
		director.match_suspended(match);
		for (int slot = -1; slot <= input_slots::keyboard_and_mouse; ++slot)
		{
			require(!director.drives_slot(slot), "Release drives no seat, so a missing pad always pauses");
		}

		std::cout << (background_only ? "PASS debug console background-only (Release): " :
			"PASS debug console (Release): ") << "inert director/background hooks and shared layout; one stderr line for "
			"'+' arguments; pad chord, overlays and unwinding inert; no developer filter or "
			"driven slot; no console, registry, menu, command, developer bot, replay or tuning classes "
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
