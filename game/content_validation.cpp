#include "game/pch.h"
#include "game/content_validation.h"
#include "game/drawable_text.h"
#include "game/objects/level_object_builder.h"

using namespace labrador;

void validate_game_content(const GameTuning& tuning, const Presentation& presentation,
	RenderResources* render, const AudioResources* audio,
	const Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
	const PlaySettings& play_settings)
{
	// Validate resource names before a menu or match can use them.
	// Optional silent banks remain valid; these checks do not play audio.
	tuning.validate_resources(*render, *audio);
	presentation.validate_resources(*render, *audio);

	// Stage selection requires at least one level.
	if (stages.empty())
	{
		throw std::runtime_error(
			"no levels: the manifest names no assets of kind 'level', so "
			"there is no stage to play.");
	}

	// All asset kinds are available now. Check every stage's actual object
	// construction and resource references before opening the first menu.
	const LevelObjectBuilder content_validator(render, audio, &play_settings,
		&tuning.paint());
	for (const Stage& stage : stages)
	{
		level_infos.get(stage.name)->validate_content(content_validator, *render, *audio);
		tuning.validate_match_sounds(*render, *audio,
			level_infos.get(stage.name)->sound_bank_name());
	}

	// Check authored weapon descriptions and stage names against their loaded
	// fonts, in addition to the character-region checks performed during parsing.
	const MenuStyle& menu = presentation.menu();
	const MainMenuPresentation& main_menu = presentation.main_menu();

	for (size_t i = 0; i < main_menu.weapon_descriptions.size(); i++)
	{
		drawable_text::check_font_can_draw(render,
			main_menu.weapon_description_font,
			main_menu.weapon_descriptions[i],
			"the weapon description at index " + std::to_string(i));
	}

	for (const Stage& stage : stages)
	{
		drawable_text::check_font_can_draw(render,
			menu.item_font,
			drawable_text::widen_drawable(stage.display_name,
				"the display_name of stage '" + stage.name + "'"),
			"the display_name of stage '" + stage.name + "'");
	}
}
