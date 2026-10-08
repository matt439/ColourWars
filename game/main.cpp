#include "game/pch.h"
#include "engine/app/application.h"
#include "engine/app/content_root.h"
#include "engine/core/registry.h"
#include "game/directory_consts.h"
#include "game/drawable_text.h"
#include "game/game_data.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
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
		const GameTuning tuning(
			resolved_under(content_root, TUNING_PATH).c_str());

		const Presentation presentation(
			resolved_under(content_root, PRESENTATION_PATH).c_str());

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
						info->icon_frame() });
					level_infos.add(name, std::move(info));
				},
				nullptr
			});

		// Asset directories resolve relative to the manifest.
		app.load_manifest(MANIFEST_PATH);

		// Validate resource names before a menu or match can use them.
		// Optional silent banks remain valid; these checks do not play audio.
		tuning.validate_resources(*app.render_resources(), *app.audio_resources());
		presentation.validate_resources(*app.render_resources(), *app.audio_resources());

		// Stage selection requires at least one level.
		if (stages.empty())
		{
			throw std::runtime_error(
				"no levels: the manifest names no assets of kind 'level', so "
				"there is no stage to play.");
		}

		// All asset kinds are available now. Check every stage's actual object
		// construction and resource references before opening the first menu.
		const LevelObjectBuilder content_validator(app.render_resources(),
			app.audio_resources(), &save.play_settings(), &tuning.paint());
		for (const Stage& stage : stages)
		{
			level_infos.get(stage.name)->validate_content(content_validator,
				*app.render_resources(), *app.audio_resources());
			tuning.validate_match_sounds(*app.render_resources(), *app.audio_resources(),
				level_infos.get(stage.name)->sound_bank_name());
		}

		// Check authored weapon descriptions and stage names against their loaded
		// fonts, in addition to the character-region checks performed during parsing.
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
		// Report failures visibly even when the process has no console.
		fprintf(stderr, "startup failure: %s\n", e.what());
		MessageBoxA(nullptr, e.what(), "ColourWars - startup failure",
			MB_OK | MB_ICONERROR);
		return 1;
	}
}
