#include "game/pch.h"
#include "game/content_validation.h"
#include "game/drawable_text.h"
#include "game/objects/level_object_builder.h"
#include "game/resource_validation.h"
#include "game/text_wrap.h"
#include "game/tunnelling_limits.h"
#include "game/team_gate_contract.h"
#include "game/objects/team_gate_structure_paintable.h"

using namespace labrador;

namespace
{
	// A gate closed to a team would push a player spawned inside it out, or
	// through it. The builder has already refused a malformed gate record.
	void check_stage_gates(const GameTuning& tuning, const LevelObjectBuilder& builder,
		const LevelLoadedInfo& info)
	{
		const JsonValue objects = info.collision_objects_json();
		bool any = false;
		for (size_t index = 0; index < objects.size(); ++index) { any = any || objects.at(index).has("gate"); }
		if (!any) { return; }
		const mattmath::Vector2F body = tuning.player().size;
		std::vector<mattmath::Vector2F> spawns = info.team_a_spawns();
		for (const mattmath::Vector2F& spawn : info.team_b_spawns()) { spawns.push_back(spawn); }
		// The built gates, so either authored rectangle spelling is read once.
		const auto built = builder.build_collision_objects(objects, TeamColour{});
		for (size_t index = 0; index < built.size(); ++index)
		{
			const Structure* structure = Collider::of(*built[index]).structure();
			const TeamGateStructurePaintable* gate = structure != nullptr ? structure->gate() : nullptr;
			if (gate == nullptr) { continue; }
			const mattmath::RectangleF bounds = gate->bounds();
			for (const mattmath::Vector2F& spawn : spawns)
			{
				try
				{
					team_gate_contract::require_clear_of_spawn({ bounds.x, bounds.y, bounds.width, bounds.height },
						{ spawn.x, spawn.y, body.x, body.y });
				}
				catch (const std::invalid_argument& error)
				{
					throw std::runtime_error(objects.at(index).where() + ": gate: " + error.what());
				}
			}
		}
	}
}

void validate_stage_objects(const GameTuning& tuning, const LevelObjectBuilder& builder,
	RenderResources& render, const AudioResources& audio, const LevelLoadedInfo& info)
{
	info.validate_content(builder, render, audio);
	check_stage_tunnelling(tuning, builder, info.collision_objects_json());
	check_stage_gates(tuning, builder, info);
	tuning.validate_match_sounds(render, audio, info.sound_bank_name());
}

void validate_stage_text(const GameTuning& tuning, const Presentation& presentation,
	RenderResources* render, const LevelLoadedInfo& info, const Stage& stage)
{
	const MenuStyle& menu = presentation.menu();
	const std::string where = "the display_name of stage '" + stage.name + "'";
	const std::wstring name = drawable_text::widen_drawable(stage.display_name, where);
	drawable_text::check_font_can_draw(render, menu.item_font, name, where);
	drawable_text::check_font_can_draw(render, menu.heading_font, name, where);
	// Practice lesson cards: the reminder's heading and controls fonts.
	for (const LevelLesson& lesson : info.lessons())
	{
		const std::string lesson_where = "the lesson at station '" + lesson.station +
			"' of stage '" + stage.name + "'";
		drawable_text::check_font_can_draw(render, menu.detail_font,
			drawable_text::widen_drawable(lesson.title, lesson_where), lesson_where + ": title");
		drawable_text::check_font_can_draw(render, tuning.match().controls_font,
			drawable_text::widen_drawable(lesson.text, lesson_where), lesson_where + ": text");
	}
}

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
	// Counts belong to the actual manifest, including explicitly selectable
	// stages excluded from Random. Check every reachable numbered label before
	// the first page, and again through the same Debug reload validation path.
	presentation.validate_stage_counts(*render, stages.size(), random_stage_indices(stages).size());

	// All asset kinds are available now. Check every stage's actual object
	// construction and resource references before opening the first menu,
	// then that a player at its speed cap cannot step through what was built.
	const LevelObjectBuilder content_validator(render, audio, &play_settings,
		&tuning.paint());
	for (const Stage& stage : stages)
	{
		validate_stage_objects(tuning, content_validator, *render, *audio,
			*level_infos.get(stage.name));
	}

	// Check authored weapon descriptions and stage names against their loaded
	// fonts, in addition to the character-region checks performed during parsing.
	const MenuStyle& menu = presentation.menu();
	const MainMenuPresentation& main_menu = presentation.main_menu();

	// The page wraps each description to its column, which no break can do
	// for a single word wider than the column.
	const FontHandle description_font =
		render->resolve_sprite_font(main_menu.weapon_description_font);
	for (size_t i = 0; i < main_menu.weapon_descriptions.size(); i++)
	{
		const std::string where = "the weapon description at index " + std::to_string(i);
		drawable_text::check_font_can_draw(render,
			main_menu.weapon_description_font,
			main_menu.weapon_descriptions[i],
			where);
		text_wrap::check_fits(*render, description_font,
			main_menu.weapon_descriptions[i],
			main_menu.weapon_description_max_width, 1.0f,
			"main_menu.weapon_description.max_width: " + where);
	}

	for (const Stage& stage : stages)
	{
		validate_stage_text(tuning, presentation, render, *level_infos.get(stage.name), stage);
	}
	// Practice's pause row names each real tool.
	for (int tool = 0; tool < static_cast<int>(WeaponType::max_prim_wep); ++tool)
	{
		drawable_text::check_font_can_draw(render, menu.item_font,
			presentation.menu_text().practice_tool_label(
				main_menu.weapon_select_art.choice(static_cast<WeaponType>(tool)).name),
			"menu_text.practice_tool");
	}
}
