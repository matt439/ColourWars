#include "game/pch.h"
#include "engine/app/application.h"
#include "engine/core/registry.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/objects/level_loaded_info.h"
#include "game/states/game_states.h"
#include "game/save.h"

using namespace DirectX;
using namespace directory_consts;
using namespace artattack;

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
		Save save;
		save.load_save_file();

		// Everything the shell cannot decide for itself. The resolution and
		// the fullscreen flag come out of this game's own save file, which is
		// exactly why the engine does not read one.
		ApplicationOptions options;
		options.window_class_name = L"ArtAttackWindowClass";
		options.window_title = L"Colour Wars";
		options.resolution = save.resolution();
		options.fullscreen = save.fullscreen();
		options.target_fps = 60;
		options.max_threads = 16;

		// The game's own resource registry, beside the engine's: a level
		// definition means nothing to the engine, so the engine does not cache
		// one. Declared before the Application so that it outlives every state
		// that borrows from it.
		Registry<LevelLoadedInfo> level_infos("Level");

		// And the pickable stages, which are those same levels in the order
		// the manifest names them. There is no second list to keep in step:
		// this one is built by the loader that fills the registry above.
		StageList stages;
		GameData data;

		Application app(std::move(options));
		app.initialize(instance, show_command);

		// The engine cannot build a level and does not pretend to: the
		// manifest may say "level" because the game teaches the loader what
		// one is. Levels are not device resources, so there is no reload half
		// - a restore must leave them where they are, with every
		// LevelLoadedInfo* the game holds still pointing at them.
		app.resource_loader()->register_kind("level",
			{
				[&level_infos, &stages](const std::string& directory,
					const std::string& name)
				{
					auto info = std::make_unique<LevelLoadedInfo>(
						(directory + name + ".json").c_str());
					stages.push_back(Stage{ name, info->display_name(),
						info->icon_frame() });
					level_infos.add(name, std::move(info));
				},
				nullptr
			});

		app.load_manifest(MANIFEST_PATH);

		// A game with no levels has nothing to offer the stage-select menu,
		// and the menu would rather not discover that by dividing by zero
		// three screens later (T6).
		if (stages.empty())
		{
			throw std::runtime_error(
				"no levels: the manifest names no assets of kind 'level', so "
				"there is no stage to play.");
		}

		data.set_application(&app);
		data.set_save(&save);
		data.set_level_infos(&level_infos);
		data.set_stages(&stages);

		return app.run(std::make_unique<GameMenu>(&data));
	}
	catch (const std::exception& e)
	{
		// T6: a broken contract stops the game dead with the reason on screen,
		// never a silent abort.
		fprintf(stderr, "startup failure: %s\n", e.what());
		MessageBoxA(nullptr, e.what(), "ArtAttack - startup failure",
			MB_OK | MB_ICONERROR);
		return 1;
	}
}
