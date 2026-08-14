#include "game/pch.h"
#include "game/presentation.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"

#include <optional>
#include <stdexcept>

using namespace artattack;
using namespace mattmath;

namespace
{
	// Every read goes through JsonValue, so a missing key, a wrong type or a
	// hand-edit that broke the file is a startup failure naming the file, the
	// position in it and the key. The helpers here are for what JsonValue
	// cannot check on its own.

	Vector2F read_vector(const JsonValue& parent, const char* key)
	{
		const JsonValue vector = parent.object(key);
		return Vector2F(vector.number("x"), vector.number("y"));
	}

	bool is_hex_colour(const std::string& text)
	{
		if (text.length() != 6 && text.length() != 8)
		{
			return false;
		}
		return text.find_first_not_of("0123456789abcdefABCDEF") ==
			std::string::npos;
	}

	// A colour by the name the engine knows it by, or as RRGGBB / RRGGBBAA.
	//
	// Both forms, because the menus needed both: most are named, and the
	// heading three of the four share is (255, 127, 0), which is not a name in
	// the palette. Colour's hex constructor answers opaque black for anything
	// it cannot parse, so the validation is here rather than there - a
	// mistyped colour should stop the game with the key in the message, not
	// paint a menu black.
	Colour read_colour(const JsonValue& parent, const char* key)
	{
		const std::string text = parent.string(key);

		const std::optional<Colour> named = colour_from_name(text);
		if (named.has_value())
		{
			return *named;
		}

		if (is_hex_colour(text))
		{
			return Colour(text);
		}

		throw std::runtime_error(parent.where() + ": '" + text +
			"' is not a colour this engine knows by name, and is not six or "
			"eight hexadecimal digits.");
	}

	PresentationSound read_sound(const JsonValue& parent, const char* key)
	{
		const JsonValue sound = parent.object(key);

		PresentationSound result;
		result.name = sound.string("name");
		result.volume = sound.number("volume");
		return result;
	}

	WidgetLayout read_layout(const JsonValue& parent, const char* key)
	{
		const JsonValue json = parent.object(key);

		WidgetLayout layout;
		layout.position = read_vector(json, "position");
		layout.size = read_vector(json, "size");
		layout.spacing = read_vector(json, "spacing");
		return layout;
	}

	SelectPresentation read_select(const JsonValue& parent, const char* key)
	{
		const JsonValue json = parent.object(key);

		SelectPresentation select;
		select.spacing = read_vector(json, "spacing");
		select.size = read_vector(json, "size");
		select.selected = read_colour(json, "selected");
		select.unselected = read_colour(json, "unselected");
		return select;
	}

	MenuStyle read_menu_style(const JsonValue& root)
	{
		const JsonValue json = root.object("menu_style");

		MenuStyle style;

		const JsonValue fonts = json.object("fonts");
		style.heading_font = fonts.string("heading");
		style.item_font = fonts.string("item");
		style.detail_font = fonts.string("detail");

		const JsonValue shadow = json.object("shadow");
		style.shadow_colour = read_colour(shadow, "colour");
		style.title_shadow_offset = read_vector(shadow, "title_offset");
		style.heading_shadow_offset = read_vector(shadow, "heading_offset");
		style.item_shadow_offset = read_vector(shadow, "item_offset");
		style.detail_shadow_offset = read_vector(shadow, "detail_offset");

		style.highlight = read_colour(json, "highlight");
		style.unhighlight = read_colour(json, "unhighlight");
		style.heading_colour = read_colour(json, "heading_colour");
		style.box_size = read_vector(json, "box_size");

		style.sound_bank = json.string("sound_bank");

		const JsonValue sounds = json.object("sounds");
		style.direction_sound = sounds.string("direction");
		style.confirm_sound = sounds.string("confirm");
		style.cancel_sound = sounds.string("cancel");

		return style;
	}

	MainMenuPresentation read_main_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("main_menu");

		MainMenuPresentation menu;

		const JsonValue fonts = json.object("fonts");
		menu.title_font = fonts.string("title");
		menu.announcement_font = fonts.string("announcement");
		menu.weapon_description_font = fonts.string("weapon_description");

		menu.title_text_colour = read_colour(json, "title_text_colour");
		menu.title_start_text_colour =
			read_colour(json, "title_start_text_colour");
		menu.title_no_pad_text_colour =
			read_colour(json, "title_no_pad_text_colour");

		const JsonValue backgrounds = json.object("backgrounds");
		menu.title_background_colour = read_colour(backgrounds, "title");
		menu.home_background_colour = read_colour(backgrounds, "home");
		menu.options_background_colour = read_colour(backgrounds, "options");
		menu.play_background_colour = read_colour(backgrounds, "play");

		menu.options_value_colour = read_colour(json, "options_value_colour");

		const JsonValue description = json.object("weapon_description");
		menu.weapon_description_colour = read_colour(description, "colour");
		menu.weapon_description_shadow_offset =
			read_vector(description, "shadow_offset");
		menu.weapon_description_x_offset = description.number("x_offset");

		menu.mode_select_spacing = read_vector(json, "mode_select_spacing");
		menu.player_count_spacing = read_vector(json, "player_count_spacing");

		menu.team_select = read_select(json, "team_select");
		menu.weapon_select = read_select(json, "weapon_select");
		menu.stage_select = read_select(json, "stage_select");

		menu.music = read_sound(json, "music");

		const JsonValue sounds = json.object("sounds");
		menu.error_sound = sounds.string("error");
		menu.ready_sound = sounds.string("ready");

		// Keyed by the weapon's name rather than written in enum order, for
		// the same reason tuning.json's weapon records are: the file names
		// what it means, and a misspelt weapon is a startup failure rather
		// than a description silently attached to the wrong pick.
		const JsonValue descriptions = json.object("weapon_descriptions");
		const auto describe = [&descriptions, &menu](WeaponType type,
			const char* key)
		{
			menu.weapon_descriptions[static_cast<size_t>(type)] =
				widen(descriptions.string(key));
		};

		describe(WeaponType::sprayer, "sprayer");
		describe(WeaponType::sniper, "sniper");
		describe(WeaponType::roller, "roller");
		describe(WeaponType::mister, "mister");
		describe(WeaponType::bucket, "bucket");
		describe(WeaponType::random_primary, "random");

		return menu;
	}

	PauseMenuPresentation read_pause_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("pause_menu");

		PauseMenuPresentation menu;
		menu.box_colour = read_colour(json, "box_colour");
		menu.layout = read_layout(json, "layout");
		menu.window_open_sound = json.string("window_open_sound");
		return menu;
	}

	EndMenuPresentation read_end_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("end_menu");

		EndMenuPresentation menu;
		menu.box_colour = read_colour(json, "box_colour");
		menu.layout = read_layout(json, "layout");
		return menu;
	}

	ResultsMenuPresentation read_results_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("results_menu");

		ResultsMenuPresentation menu;
		menu.box_colour = read_colour(json, "box_colour");
		menu.heading_colour = read_colour(json, "heading_colour");

		const JsonValue fill = json.object("fill");
		menu.fill_box_size = read_vector(fill, "box_size");
		menu.fill_box_y_offset = fill.number("box_y_offset");
		menu.team_fill_border = read_vector(fill, "border");
		menu.team_fill_delay = fill.number("delay");
		menu.team_fill_time = fill.number("time");

		// Derived rather than stated, which is what the header did too - and
		// stating it as well would be a second number to keep in step with
		// the first two.
		menu.team_fill_size =
			menu.fill_box_size - menu.team_fill_border * 2.0f;

		menu.show_results_delay = json.number("show_results_delay");

		menu.percentage_colour = read_colour(json, "percentage_colour");
		menu.team_b_percentage_x_offset =
			json.number("team_b_percentage_x_offset");

		const JsonValue team_mark = json.object("team_mark");
		menu.team_mark_size = read_vector(team_mark, "size");
		menu.team_mark_gap = team_mark.number("gap");

		const JsonValue positions = json.object("positions");
		menu.proceed_position = read_vector(positions, "proceed");
		menu.heading_position = read_vector(positions, "heading");
		menu.winner_position = read_vector(positions, "winner");

		const JsonValue sounds = json.object("sounds");
		menu.winner_sound = read_sound(sounds, "winner");
		menu.fill_sound = read_sound(sounds, "fill");

		return menu;
	}

	HudPresentation read_hud(const JsonValue& root)
	{
		const JsonValue json = root.object("hud");

		HudPresentation hud;
		hud.right_margin = json.number("right_margin");
		hud.ammo_top_margin = json.number("ammo_top_margin");
		hud.health_top_margin = json.number("health_top_margin");
		hud.box_size = read_vector(json, "box_size");
		hud.box_fill_border = json.number("box_fill_border");
		hud.health_colour = read_colour(json, "health_colour");

		hud.sprite_sheet = json.string("sprite_sheet");
		hud.empty_box_frame = json.string("empty_box_frame");
		hud.fill_frame = json.string("fill_frame");

		const JsonValue timer = json.object("timer");
		hud.timer_font = timer.string("font");
		hud.timer_offset = read_vector(timer, "offset");
		hud.timer_shadow_offset = read_vector(timer, "shadow_offset");
		hud.timer_colour = read_colour(timer, "colour");
		hud.timer_shadow_colour = read_colour(timer, "shadow_colour");
		hud.timer_reference_width = timer.number("reference_width");

		const JsonValue respawn = json.object("respawn_timer");
		hud.respawn_font = respawn.string("font");
		hud.respawn_offset = read_vector(respawn, "offset");
		hud.respawn_shadow_offset = read_vector(respawn, "shadow_offset");
		hud.respawn_colour = read_colour(respawn, "colour");
		hud.respawn_shadow_colour = read_colour(respawn, "shadow_colour");

		return hud;
	}

	DebugOverlayPresentation read_debug_overlay(const JsonValue& root)
	{
		const JsonValue json = root.object("debug_overlay");

		DebugOverlayPresentation debug;
		debug.font = json.string("font");
		debug.font_scale = json.number("font_scale");
		debug.line_spacing = json.number("line_spacing");
		debug.position = read_vector(json, "position");
		debug.colour = read_colour(json, "colour");
		debug.shadow_colour = read_colour(json, "shadow_colour");
		debug.shadow_offset = read_vector(json, "shadow_offset");
		debug.shadow_scale = json.number("shadow_scale");
		return debug;
	}
}

Presentation::Presentation(const char* path)
{
	const JsonDocument document = read_json_file(path);
	const JsonValue root = document.root();

	this->menu_ = read_menu_style(root);
	this->main_menu_ = read_main_menu(root);
	this->pause_menu_ = read_pause_menu(root);
	this->end_menu_ = read_end_menu(root);
	this->results_menu_ = read_results_menu(root);
	this->hud_ = read_hud(root);
	this->debug_overlay_ = read_debug_overlay(root);
}

const MenuStyle& Presentation::menu() const
{
	return this->menu_;
}

const MainMenuPresentation& Presentation::main_menu() const
{
	return this->main_menu_;
}

const PauseMenuPresentation& Presentation::pause_menu() const
{
	return this->pause_menu_;
}

const EndMenuPresentation& Presentation::end_menu() const
{
	return this->end_menu_;
}

const ResultsMenuPresentation& Presentation::results_menu() const
{
	return this->results_menu_;
}

const HudPresentation& Presentation::hud() const
{
	return this->hud_;
}

const DebugOverlayPresentation& Presentation::debug_overlay() const
{
	return this->debug_overlay_;
}
