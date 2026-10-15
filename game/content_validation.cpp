#include "game/pch.h"
#include "game/content_validation.h"
#include "game/drawable_text.h"
#include "game/objects/level_object_builder.h"
#include "game/resource_validation.h"

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
	const ResourceValidation check("presentation.json: tool_motion", *render, *audio);
	const std::array<const char*, 5> tools = { "sprayer", "sniper", "roller", "mister", "bucket" };
	for (size_t index = 0; index < tools.size(); ++index)
	{
		const WeaponDetails& details = tuning.weapon(static_cast<WeaponType>(index)).details;
		const SpriteSheet* sheet = render->sprite_sheet(details.sheet_name);
		const SpriteFrame& idle = sheet->sprite_frame(sheet->resolve_sprite_frame(details.frame_name));
		for (const char* suffix : { "", "_motion_1", "_motion_2", "_lift" })
		{
			const std::string name = details.frame_name + suffix;
			for (const std::string& frame : { name, name + "_paint" })
			{
				const std::string key = std::string(tools[index]) + "." + frame;
				check.frame(key, details.sheet_name, frame);
				const SpriteFrame& pose = sheet->sprite_frame(sheet->resolve_sprite_frame(frame));
				if (pose.source_rectangle().size() != idle.source_rectangle().size() ||
					pose.origin() != idle.origin())
				{
					throw std::runtime_error("presentation.json: tool_motion." + key +
						": pose and paint overlay must retain idle dimensions and origin.");
				}
			}
		}
		if (static_cast<WeaponType>(index) == WeaponType::sprayer ||
			static_cast<WeaponType>(index) == WeaponType::mister)
		{
			const std::string name = details.frame_name + "_supply";
			check.animation(name, details.sheet_name, name);
			const AnimationStrip& supply = sheet->animation_strip(sheet->resolve_animation_strip(name));
			if (supply.frame_count() < 2)
			{
				throw std::runtime_error("weapon supply " + name + ": needs empty and full levels.");
			}
			for (int level = 0; level < supply.frame_count(); ++level)
			{
				if (supply.frame_rect(level).size() != idle.source_rectangle().size() ||
					idle.origin() != mattmath::Vector2F::ZERO)
				{
					throw std::runtime_error("weapon supply " + name +
						": levels must retain the body canvas and zero authored origin.");
				}
			}
		}
	}

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
