#include "game/pch.h"
#include "engine/app/application.h"
#include "engine/app/content_root.h"
#include "engine/core/file_path.h"
#include "game/states/menu_content_validation.h"
#include "game/directory_consts.h"
#include "game/fatal_report.h"
#include "game/debug/debug_director.h"
#include "game/game_data.h"
#include "game/presentation.h"
#include "game/tuning.h"
#include "game/states/game_states.h"
#include "game/save.h"
#include "game/simulation.h"
#include "game/sound_groups.h"
#include "game/stage_catalog.h"
#include "game/stage_loading.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using namespace directory_consts;
using namespace labrador;

// Indicates to hybrid graphics systems to prefer the discrete part by default.
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	FailurePhase phase = FailurePhase::preferences;
	try
	{
		// Seed before stage, weapon, or palette selection uses rand().
		std::srand(static_cast<unsigned int>(std::time(nullptr)));

		Save save;
		save.load_save_file();

		ApplicationOptions options;
		options.window_class_name = L"ColourWarsWindowClass";
		options.window_title = L"Colour Wars";
		options.resolution = save.resolution();
		options.fullscreen = save.fullscreen();
		options.target_fps = SIMULATION_HZ;
		options.max_threads = 16;

		// Declare borrowed game resources before Application so they outlive its states.
		// The stage list and level registry, filled in manifest order.
		StageCatalog stage_catalog;

		// The player's stage folder, attached once the manifest has loaded.
		// GameMenu lends it to Stage Select, which rescans as it opens.
		UserStageFolder user_stages;

		// Resolve early content reads beside the executable, as load_manifest does.
		const std::string content_root = executable_directory();

		// Parse tuning and presentation before creating states. Their asset names
		// are validated after the manifest loads; both objects outlive Application.
		// Mutable only through the developer director (Debug `reload` and
		// `feedback`); GameData hands everything else const pointers.
		phase = FailurePhase::definitions;
		GameTuning tuning(
			resolved_under(content_root, TUNING_PATH).c_str());

		Presentation presentation(
			resolved_under(content_root, PRESENTATION_PATH).c_str());

		// Inert in Release. Declared before Application so it outlives every state.
		DebugDirector debug(&tuning, &presentation);

		GameData data;

		phase = FailurePhase::initialization;
		Application app(std::move(options));
		app.initialize();

		// Register the game's level resource kind. Level definitions survive device
		// restoration, so this kind has no reload callback.
		app.resource_loader()->register_kind("level",
			{
				// Declared levels are required even if marked optional in the manifest.
				[&stage_catalog](const std::string& directory,
					const std::string& name, bool /*optional*/)
				{
					stage_catalog.add(read_stage_definition(directory + name + ".json", name));
				},
				nullptr
			});

		// Asset directories resolve relative to the manifest.
		phase = FailurePhase::manifest;
		app.load_manifest(MANIFEST_PATH);

		// The player's own stages join after the manifest's, each checked on its
		// own: a broken one is refused with one stderr line and startup goes on.
		user_stages.attach(path_from_utf8(user_stage_directory()),
			UserStageChecks{ &tuning, &presentation, app.render_resources(),
				app.audio_resources(), &save.play_settings() },
			app.audio_resources(), &stage_catalog);
		user_stages.scan();

		// Check every name, stage and authored string against the loaded assets
		// before the first menu. The Debug `reload` command runs the same checks.
		phase = FailurePhase::validation;
		validate_shell_content(tuning, presentation, app.render_resources(),
			app.audio_resources(), stage_catalog.level_infos(), stage_catalog.stages(),
			save.play_settings());
		// The saved Music and SFX volumes are group volumes from here on;
		// the Controls page pushes a change through the same call.
		sound_groups::assign(*app.audio_resources(), tuning, presentation,
			stage_catalog.level_infos(), stage_catalog.stages());
		sound_groups::apply(*app.audio_resources(), save.play_settings());

		data.set_application(&app);
		data.set_save(&save);
		data.set_stage_catalog(&stage_catalog);
		data.set_tuning(&tuning);
		data.set_presentation(&presentation);
		data.set_debug(&debug);
		debug.attach(&app, &data);

		// "+stage movement_course +players 4" and the like. The CRT has already
		// split the command line for this wide-entry program.
		std::vector<std::wstring> arguments;
		if (__wargv != nullptr)
		{
			for (int i = 1; i < __argc; ++i)
			{
				arguments.emplace_back(__wargv[i]);
			}
		}
		phase = FailurePhase::launch;
		debug.run_launch_arguments(arguments);

		phase = FailurePhase::running;
		return app.run(std::make_unique<GameMenu>(&data, &user_stages));
	}
	catch (...)
	{
		return present_fatal_error(phase, std::current_exception());
	}
}
