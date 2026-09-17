#include "game/pch.h"
#include "engine/app/application.h"
#include "engine/app/content_root.h"
#include "engine/core/registry.h"
#include "game/directory_consts.h"
#include "game/drawable_text.h"
#include "game/game_data.h"
#include "game/objects/level_loaded_info.h"
#include "game/presentation.h"
#include "game/tuning.h"
#include "game/states/game_states.h"
#include "game/save.h"
#include <cstdlib>
#include <ctime>

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
		// Every random choice the game makes - the random stage, the random
		// primary weapon, the team colours a match is dealt - goes through
		// rand(), and an unseeded CRT behaves exactly as srand(1): the same
		// stage, the same weapon and the same colours on every launch. This is
		// the one place that runs before any of them, so it is the one place
		// that seeds.
		std::srand(static_cast<unsigned int>(std::time(nullptr)));

		Save save;
		save.load_save_file();

		// Everything the shell cannot decide for itself. The resolution and
		// the fullscreen flag come out of this game's own save file, which is
		// exactly why the engine does not read one.
		ApplicationOptions options;
		options.window_class_name = L"ColourWarsWindowClass";
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

		// Where the content is: beside the executable, wherever the game was
		// started from. That is the engine's rule (engine/app/content_root.h)
		// and load_manifest applies it to the manifest's path itself - but
		// the two files below are read before the manifest, so it is applied
		// here by hand, with the same pieces. Without this they would resolve
		// against the working directory while the manifest did not, and a
		// game started from a shortcut would fail on tuning.json with the
		// manifest sitting right there beside it.
		const std::string content_root = executable_directory();

		// Every number a paint match is balanced on. Declared here for the
		// same reason as the registry above: a level's players, weapons and
		// projectiles all borrow from it, so it has to outlive the
		// Application. Read before the manifest because it names no asset -
		// it is the file the manifest's assets are used *by*.
		const GameTuning tuning(
			resolved_under(content_root, TUNING_PATH).c_str());

		// And what the shell looks like: every font, colour, offset and box
		// size the menus, the HUD and the debug overlay are laid out from.
		// Read here, beside the tuning, for the same reason - a menu page
		// borrows it and every page outlives none of this.
		const Presentation presentation(
			resolved_under(content_root, PRESENTATION_PATH).c_str());

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
				// A level is never optional: a stage the manifest names and
				// the disk does not have is a content bug, and the stage-select
				// menu would find out three screens later.
				[&level_infos, &stages](const std::string& directory,
					const std::string& name, bool /*optional*/)
				{
					auto info = std::make_unique<LevelLoadedInfo>(
						(directory + name + ".json").c_str());

					// Before it joins the stage list, and for the same reason
					// the disk is checked here rather than at the menu: every
					// other key in the file is read for the first time inside
					// GameLevel::init(), which is after four people have
					// chosen a stage and pressed A.
					info->validate();

					// The stage name is the one level value that is drawn as
					// text rather than used as a key, so it is the one that
					// can carry a character no font here has a glyph for. The
					// result is discarded: the menu widens it again at the
					// point it draws it, and this is the check.
					drawable_text::widen_drawable(info->display_name(),
						"the display_name in " + directory + name + ".json");

					stages.push_back(Stage{ name, info->display_name(),
						info->icon_frame() });
					level_infos.add(name, std::move(info));
				},
				nullptr
			});

		// Resolved beside the executable by the engine, and every relative
		// directory inside it beside the manifest - so the fonts, textures,
		// levels and sounds it names come out of the same staged mirror the
		// two files above were read from.
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

		// The content strings, now asked of the fonts that will draw them.
		//
		// This is the earliest it can be done: presentation.json and every
		// level file were read above, before the manifest, so nothing had a
		// font to ask at the time. Both of those checks were the region
		// constant in drawable_text - a claim about all seven .spritefonts,
		// made by decoding their headers by hand - and this is the same
		// question put to the atlas the engine actually loaded.
		//
		// Only the two authored strings. Every other wide literal in this
		// repository is typed into a source file that is plain ASCII by
		// policy, and a font that could not draw those could not draw the
		// word "Start".
		{
			const MenuStyle& menu = presentation.menu();
			const MainMenuPresentation& main_menu = presentation.main_menu();

			for (size_t i = 0; i < main_menu.weapon_descriptions.size(); i++)
			{
				drawable_text::check_font_can_draw(app.render_resources(),
					main_menu.weapon_description_font,
					main_menu.weapon_descriptions[i],
					"the weapon description at index " + std::to_string(i));
			}

			for (const Stage& stage : stages)
			{
				drawable_text::check_font_can_draw(app.render_resources(),
					menu.item_font,
					drawable_text::widen_drawable(stage.display_name,
						"the display_name of stage '" + stage.name + "'"),
					"the display_name of stage '" + stage.name + "'");
			}
		}

		data.set_application(&app);
		data.set_save(&save);
		data.set_level_infos(&level_infos);
		data.set_stages(&stages);
		data.set_tuning(&tuning);
		data.set_presentation(&presentation);

		return app.run(std::make_unique<GameMenu>(&data));
	}
	catch (const std::exception& e)
	{
		// T6: a broken contract stops the game dead with the reason on screen,
		// never a silent abort.
		fprintf(stderr, "startup failure: %s\n", e.what());
		MessageBoxA(nullptr, e.what(), "ColourWars - startup failure",
			MB_OK | MB_ICONERROR);
		return 1;
	}
}
