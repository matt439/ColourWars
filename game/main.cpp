#include "game/pch.h"
#include "engine/app/application.h"
#include "engine/app/content_root.h"
#include "engine/core/registry.h"
#include "game/content_validation.h"
#include "game/directory_consts.h"
#include "game/drawable_text.h"
#include "game/debug/debug_director.h"
#include "game/game_data.h"
#include "game/objects/level_loaded_info.h"
#include "game/presentation.h"
#include "game/tuning.h"
#include "game/states/game_states.h"
#include "game/save.h"
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using namespace DirectX;
using namespace directory_consts;
using namespace labrador;

// Indicates to hybrid graphics systems to prefer the discrete part by default.
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ LPWSTR,
	_In_ int show_command)
{
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
		options.target_fps = 60;
		options.max_threads = 16;

		// Declare borrowed game resources before Application so they outlive its states.
		Registry<LevelLoadedInfo> level_infos("Level");

		// Populated alongside the registry in manifest order.
		StageList stages;

		// Resolve early content reads beside the executable, as load_manifest does.
		const std::string content_root = executable_directory();

		// Parse tuning and presentation before creating states. Their asset names
		// are validated after the manifest loads; both objects outlive Application.
		// Mutable only through the developer director (Debug `reload` and
		// `feedback`); GameData hands everything else const pointers.
		GameTuning tuning(
			resolved_under(content_root, TUNING_PATH).c_str());

		Presentation presentation(
			resolved_under(content_root, PRESENTATION_PATH).c_str());

		// Inert in Release. Declared before Application so it outlives every state.
		DebugDirector debug(&tuning, &presentation);

		GameData data;

		Application app(std::move(options));
		app.initialize(instance, show_command);

		// Register the game's level resource kind. Level definitions survive device
		// restoration, so this kind has no reload callback.
		app.resource_loader()->register_kind("level",
			{
				// Declared levels are required even if marked optional in the manifest.
				[&level_infos, &stages](const std::string& directory,
					const std::string& name, bool /*optional*/)
				{
					auto info = std::make_unique<LevelLoadedInfo>(
						(directory + name + ".json").c_str());

					// Validate the definition before exposing it in stage selection.
					info->validate();

					// Validate authored display text now; the menu converts it again for display.
					drawable_text::widen_drawable(info->display_name(),
						"the display_name in " + directory + name + ".json");

					stages.push_back(Stage{ name, info->display_name(),
						info->icon_frame(), info->include_in_random() });
					level_infos.add(name, std::move(info));
				},
				nullptr
			});

		// Asset directories resolve relative to the manifest.
		app.load_manifest(MANIFEST_PATH);

		// Check every name, stage and authored string against the loaded assets
		// before the first menu. The Debug `reload` command runs the same checks.
		validate_game_content(tuning, presentation, app.render_resources(),
			app.audio_resources(), level_infos, stages, save.play_settings());

		data.set_application(&app);
		data.set_save(&save);
		data.set_level_infos(&level_infos);
		data.set_stages(&stages);
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
		debug.run_launch_arguments(arguments);

		return app.run(std::make_unique<GameMenu>(&data));
	}
	catch (const std::exception& e)
	{
		// Report failures visibly even when the process has no console.
		fprintf(stderr, "startup failure: %s\n", e.what());
		MessageBoxA(nullptr, e.what(), "ColourWars - startup failure",
			MB_OK | MB_ICONERROR);
		return 1;
	}
}
