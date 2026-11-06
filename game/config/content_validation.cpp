#include "game/pch.h"
#include "game/config/content_validation.h"
#include "game/config/drawable_text.h"
#include "game/input/controls_text.h"
#include "game/objects/figure_person.h"
#include "game/objects/level_object_builder.h"
#include "game/config/resource_validation.h"
#include "game/config/text_wrap.h"
#include "game/config/tunnelling_limits.h"
#include "game/contracts/team_gate_contract.h"
#include "game/objects/team_gate_structure_paintable.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

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
		const labrador::Vector2F body = tuning.player().size;
		std::vector<labrador::Vector2F> spawns = info.team_a_spawns();
		for (const labrador::Vector2F& spawn : info.team_b_spawns()) { spawns.push_back(spawn); }
		// The built gates, so either authored rectangle spelling is read once.
		const auto built = builder.build_collision_objects(objects, TeamColour{});
		for (size_t index = 0; index < built.size(); ++index)
		{
			const Structure* structure = Collider::of(*built[index]).structure();
			const TeamGateStructurePaintable* gate = structure != nullptr ? structure->gate() : nullptr;
			if (gate == nullptr) { continue; }
			const labrador::RectangleF bounds = gate->bounds();
			for (const labrador::Vector2F& spawn : spawns)
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

	// What Stage Select and the countdown card draw for a shipped stage.
	const std::wstring& shipped_stage_name(const Presentation& presentation, const std::string& stage)
	{
		const std::wstring* name = presentation.menu_text().stage_name(stage);
		if (name == nullptr)
		{
			throw std::runtime_error("presentation.json: menu_text.stage_names has no '" + stage +
				"', a stage the manifest lists. Add it with the level's display_name.");
		}
		return *name;
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

void validate_stage_text(const Presentation& presentation,
	RenderResources* render, const LevelLoadedInfo& info, const Stage& stage)
{
	const MenuStyle& menu = presentation.menu();
	const std::string authored_where = "the display_name of stage '" + stage.name + "'";
	const std::wstring authored = drawable_text::widen_drawable(stage.display_name, authored_where);
	// What Stage Select and the countdown card draw: a shipped stage's
	// menu_text.stage_names entry, a user stage's own display_name.
	const std::wstring* shipped = stage.user ? nullptr : &shipped_stage_name(presentation, stage.name);
	const std::wstring& name = shipped != nullptr ? *shipped : authored;
	const std::string where = shipped != nullptr ?
		"presentation.json: menu_text.stage_names." + stage.name : authored_where;
	drawable_text::check_font_can_draw(render, menu.item_font, name, where);
	drawable_text::check_font_can_draw(render, menu.heading_font, name, where);
	// Practice lesson cards: the reminder's heading and controls fonts.
	for (const LevelLesson& lesson : info.lessons())
	{
		const std::string lesson_where = "the lesson at station '" + lesson.station +
			"' of stage '" + stage.name + "'";
		drawable_text::check_font_can_draw(render, menu.detail_font,
			drawable_text::widen_drawable(lesson.title, lesson_where), lesson_where + ": title");
		drawable_text::check_font_can_draw(render, presentation.match().controls_text.font,
			drawable_text::widen_drawable(lesson.text, lesson_where), lesson_where + ": text");
	}
}

void validate_stage_names(const Presentation& presentation, const StageList& stages)
{
	for (const Stage& stage : stages)
	{
		if (stage.user) { continue; }
		if (!stage.name.empty() && stage.name.front() == '_')
		{
			throw std::runtime_error("stage '" + stage.name + "': a shipped stage's name must not "
				"start with '_', which presentation.json's menu_text.stage_names reads as a comment.");
		}
		std::ignore = shipped_stage_name(presentation, stage.name);
	}
	for (const auto& entry : presentation.menu_text().stage_names)
	{
		const auto listed = std::find_if(stages.begin(), stages.end(),
			[&entry](const Stage& stage) { return !stage.user && stage.name == entry.first; });
		if (listed == stages.end())
		{
			throw std::runtime_error("presentation.json: menu_text.stage_names has '" + entry.first +
				"', which is not a stage the manifest lists. Remove it, or register the stage.");
		}
	}
}

void check_stage_names_agree(const JsonValue& base_presentation, const StageList& stages)
{
	const JsonValue names = base_presentation.object("menu_text").object("stage_names");
	for (const Stage& stage : stages)
	{
		if (stage.user || !names.has(stage.name.c_str())) { continue; }
		const std::string base = names.string(stage.name.c_str());
		if (base != stage.display_name)
		{
			throw std::runtime_error(names.where() + ": '" + stage.name + "' is '" + base +
				"', but the display_name in levels/" + stage.name + ".json is '" +
				stage.display_name + "'; the two must agree.");
		}
	}
}

void check_stage_names_agree(const std::string& presentation_path, const StageList& stages)
{
	const JsonDocument base = read_json_file(presentation_path.c_str());
	check_stage_names_agree(base.root(), stages);
}

void validate_controls_reminders(const Presentation& presentation, const RenderResources& render)
{
	const ControlsCopy& copy = presentation.menu_text().controls;
	const std::string& font = presentation.match().controls_text.font;
	const auto check = [&](const char* key, const std::wstring& words)
	{
		const std::string where = std::string("presentation.json: menu_text.controls.") + key +
			" in match.controls_text.font";
		drawable_text::check_font_can_draw(&render, font, words, where);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), words);
		if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
		{ throw std::runtime_error(where + " requires finite positive reminder bounds."); }
	};
	for (const Key key : controls_text::jump_keys())
	{
		check("keyboard_reminder", controls_text::keyboard_controls_reminder(copy, key));
	}
	// Every control each keyboard action can show (Shoot's mouse buttons,
	// Pause's extra key), the others at their defaults where that is valid.
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		std::vector<DesktopControl> choices;
		if (action == KeyboardAction::pause) { choices.push_back({}); }
		for (const Key key : controls_text::jump_keys()) { choices.push_back({ key, MouseButton::none }); }
		if (action == KeyboardAction::shoot)
		{
			for (const MouseButton button : shoot_mouse_buttons()) { choices.push_back({ Key::none, button }); }
		}
		for (const DesktopControl& choice : choices)
		{
			InputBindings bindings;
			set_keyboard_control(bindings, action, choice);
			if (keyboard_control_conflict(bindings, action, choice)) { continue; }
			check("keyboard_reminder", controls_text::keyboard_controls_reminder(copy, bindings));
		}
	}
	// Each pad action's every primary, with and without an alternate, the
	// others at their defaults where valid, so every pad label is drawn;
	// then the per-pad lines of four slots whose profiles differ.
	std::array<int, input_slots::pad_count> every_pad{};
	for (int slot = 0; slot < input_slots::pad_count; ++slot) { every_pad[static_cast<size_t>(slot)] = slot; }
	for (size_t index = 0; index < pad_action_count; ++index)
	{
		const PadAction action = static_cast<PadAction>(index);
		for (const PadControl primary : pad_controls())
		{
			for (const PadControl alternate : { PadControl::none, PadControl::start, PadControl::right_shoulder })
			{
				InputBindings bindings;
				for (PadProfile& pad : bindings.pads) { pad[action] = { primary, alternate }; }
				try { bindings.validate(); }
				catch (const std::invalid_argument&) { continue; }
				check("gamepad_reminder", controls_text::pad_controls_reminder(copy, bindings, every_pad));
			}
		}
	}
	InputBindings mixed;
	mixed.pads[1][PadAction::jump] = { PadControl::b, PadControl::right_shoulder };
	mixed.pads[2][PadAction::shoot] = { PadControl::left_shoulder, PadControl::none };
	mixed.pads[3][PadAction::pause] = { PadControl::y, PadControl::none };
	check("gamepad_per_pad_reminder", controls_text::pad_controls_reminder(copy, mixed, every_pad));
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
	validate_controls_reminders(presentation, *render);
	// Every person the figure slips allow has a face band over every body
	// frame (presentation.json figure.slips, built by build-painter-art.ps1),
	// so no match can draw a person the atlas lacks.
	{
		const PlayerTuning& player = tuning.player();
		const FigureSlipsPresentation& slips = presentation.figure().slips;
		const SpriteSheet* sheet = render->sprite_sheet(player.idle.sheet);
		const ResourceValidation figure("presentation.json: figure.slips", *render, *audio);
		for (const FigurePerson& person : FigurePerson::everyone(slips))
		{
			figure.frame(person.name(slips), player.idle.sheet, person.band_frame(slips, player.idle.strip, 0));
			for (const PlayerAnimationTuning* action : { &player.takeoff, &player.land, &player.fire,
				&player.hurt, &player.elimination })
			{
				const int frames = sheet->animation_strip(sheet->resolve_animation_strip(action->strip)).frame_count();
				for (int frame = 0; frame < frames; ++frame)
				{
					figure.frame(person.name(slips), action->sheet, person.band_frame(slips, action->strip, frame));
				}
			}
			// The band's frames replace exactly the band's texels.
			const labrador::RectangleI& face = sheet->sprite_frame(sheet->resolve_sprite_frame(
				person.band_frame(slips, player.idle.strip, 0))).source_rectangle();
			if (static_cast<float>(face.width) != slips.band.width ||
				static_cast<float>(face.height) != slips.band.height)
			{
				throw std::runtime_error("presentation.json: figure.slips.band must match the baked face frames (" +
					std::to_string(face.width) + "x" + std::to_string(face.height) + "); rebuild the painter art.");
			}
		}
	}
	const ResourceValidation check("presentation.json: tool_motion", *render, *audio);
	for (size_t index = 0; index < TUNED_WEAPON_COUNT; ++index)
	{
		const WeaponDetails& details = tuning.weapon(static_cast<WeaponType>(index)).details;
		const SpriteSheet* sheet = render->sprite_sheet(details.sheet_name);
		const SpriteFrame& idle = sheet->sprite_frame(sheet->resolve_sprite_frame(details.frame_name));
		for (const char* suffix : { "", "_motion_1", "_motion_2", "_lift" })
		{
			const std::string name = details.frame_name + suffix;
			for (const std::string& frame : { name, name + "_paint" })
			{
				const std::string key = std::string(WEAPON_NAMES[index]) + "." + frame;
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
			bool canvas = idle.origin() == labrador::Vector2F::zero;
			for (int level = 0; canvas && level < supply.frame_count(); ++level)
			{
				canvas = supply.frame_rect(level).size() == idle.source_rectangle().size();
			}
			if (!canvas)
			{
				throw std::runtime_error("weapon supply " + name +
					": levels must retain the body canvas and zero authored origin.");
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
	const LevelObjectBuilder content_validator(render, &play_settings,
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

	validate_stage_names(presentation, stages);
	for (const Stage& stage : stages)
	{
		validate_stage_text(presentation, render, *level_infos.get(stage.name), stage);
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
