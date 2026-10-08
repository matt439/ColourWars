#include "game/pch.h"
#include "game/presentation.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "game/drawable_text.h"
#include "game/resource_validation.h"

#include <cmath>
#include <initializer_list>
#include <optional>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

namespace
{
	// JsonValue provides source context for missing keys and wrong types.

	Vector2F read_vector(const JsonValue& parent, const char* key)
	{
		const JsonValue vector = parent.object(key);
		return Vector2F(vector.number("x"), vector.number("y"));
	}

	float read_positive(const JsonValue& parent, const char* key)
	{
		const float value = parent.number(key);
		if (!std::isfinite(value) || value <= 0.0f)
		{
			throw std::runtime_error(parent.where() + ": '" + key +
				"' must be finite and greater than zero.");
		}
		return value;
	}

	Vector2F read_positive_vector(const JsonValue& parent, const char* key)
	{
		const JsonValue vector = parent.object(key);
		return Vector2F(read_positive(vector, "x"), read_positive(vector, "y"));
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

	// Accept named colours or RRGGBB/RRGGBBAA. Validate hex strings before
	// construction so a typo cannot silently become opaque black.
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
		style.disabled = read_colour(json, "disabled");
		style.warning = read_colour(json, "warning");
		style.heading_colour = read_colour(json, "heading_colour");
		const JsonValue chrome = json.object("chrome");
		style.focus_panel_colour = read_colour(chrome, "focus_panel_colour");
		style.focus_padding = read_vector(chrome, "focus_padding");
		style.accent_width = read_positive(chrome, "accent_width");
		style.panel_edge_colour = read_colour(chrome, "panel_edge_colour");
		style.panel_shadow_offset = read_vector(chrome, "panel_shadow_offset");
		style.scrim_colour = read_colour(chrome, "scrim_colour");
		style.brand_position = read_vector(chrome, "brand_position");
		style.brand_font = chrome.string("brand_font");
		style.footer_y = chrome.number("footer_y");
		style.rule_height = read_positive(chrome, "rule_height");
		style.icon_padding = read_vector(chrome, "icon_padding");
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
		const JsonValue backdrop = json.object("backdrop");
		menu.backdrop_sheet = backdrop.string("sheet");
		menu.backdrop_frame = backdrop.string("frame");
		menu.title_position = read_vector(json, "title_position");
		menu.title_subtitle_position = read_vector(json, "title_subtitle_position");
		menu.title_start_position = read_vector(json, "title_start_position");
		menu.home_subtitle_position = read_vector(json, "home_subtitle_position");

		const JsonValue fonts = json.object("fonts");
		menu.title_font = fonts.string("title");
		menu.announcement_font = fonts.string("announcement");
		menu.weapon_description_font = fonts.string("weapon_description");
		menu.build_id_position = read_vector(json, "build_id_position");

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
		menu.options_spacing = read_vector(json, "options_spacing");

		const JsonValue description = json.object("weapon_description");
		menu.weapon_description_colour = read_colour(description, "colour");
		menu.weapon_description_shadow_offset =
			read_vector(description, "shadow_offset");
		menu.weapon_description_x_offset = description.number("x_offset");

		menu.mode_select_spacing = read_vector(json, "mode_select_spacing");
		menu.player_count_spacing = read_vector(json, "player_count_spacing");

		menu.team_select = read_select(json, "team_select");
		menu.weapon_select = read_select(json, "weapon_select");
		const JsonValue stage = json.object("stage_select");
		menu.stage_select.preview_position = read_vector(stage, "preview_position");
		menu.stage_select.preview_size = read_positive_vector(stage, "preview_size");
		menu.stage_select.name_position = read_vector(stage, "name_position");
		menu.stage_select.navigation_position = read_vector(stage, "navigation_position");
		menu.stage_select.briefing = read_layout(stage, "briefing");
		menu.stage_select.briefing.size = read_positive_vector(stage.object("briefing"), "size");
		menu.stage_select.briefing_padding = read_positive_vector(stage, "briefing_padding");
		menu.stage_select.selected = read_colour(stage, "selected");
		menu.stage_select.unselected = read_colour(stage, "unselected");

		menu.music = read_sound(json, "music");

		const JsonValue sounds = json.object("sounds");
		menu.error_sound = sounds.string("error");
		menu.ready_sound = sounds.string("ready");

		// Resolve descriptions by weapon name so file order is independent of enum order.
		const JsonValue descriptions = json.object("weapon_descriptions");
		const auto describe = [&descriptions, &menu](WeaponType type,
			const char* key)
		{
			// Normalize and validate authored display text while its source key is available.
			menu.weapon_descriptions[static_cast<size_t>(type)] =
				drawable_text::widen_drawable(descriptions.string(key),
					descriptions.where() + ": " + key);
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
		menu.box_size = read_vector(json, "box_size");
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
		menu.box_size = read_vector(json, "box_size");
		menu.session_score_position = read_vector(json, "session_score_position");
		menu.session_result_position = read_vector(json, "session_result_position");
		menu.contribution_position = read_vector(json, "contribution_position");
		menu.contribution_spacing = json.number("contribution_spacing");
		menu.contribution_font = json.string("contribution_font");
		menu.box_colour = read_colour(json, "box_colour");
		menu.heading_colour = read_colour(json, "heading_colour");

		const JsonValue fill = json.object("fill");
		menu.fill_box_size = read_vector(fill, "box_size");
		menu.fill_box_y_offset = fill.number("box_y_offset");
		menu.team_fill_border = read_vector(fill, "border");
		menu.team_fill_delay = fill.number("delay");
		// Zero has no defined fill ratio. Reject it here instead of sending a
		// first-frame NaN to the renderer.
		menu.team_fill_time = read_positive(fill, "time");

		// Derive the fill size from the authored box and border.
		menu.team_fill_size =
			menu.fill_box_size - menu.team_fill_border * 2.0f;

		menu.show_results_delay = json.number("show_results_delay");

		menu.percentage_colour = read_colour(json, "percentage_colour");

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

	PromptBarPresentation read_prompt_bar(const JsonValue& root)
	{
		const JsonValue json = root.object("prompt_bar");

		PromptBarPresentation bar;
		bar.font = json.string("font");
		bar.position = read_vector(json, "position");
		bar.colour = read_colour(json, "colour");
		bar.shadow_colour = read_colour(json, "shadow_colour");
		bar.shadow_offset = read_vector(json, "shadow_offset");
		return bar;
	}

	HudPresentation read_hud(const JsonValue& root)
	{
		const JsonValue json = root.object("hud");

		HudPresentation hud;
		hud.meter_label_font = json.string("meter_label_font");
		hud.meter_track_colour = read_colour(json, "meter_track_colour");
		hud.meter_label_gap = read_positive(json, "meter_label_gap");
		hud.reference_height = read_positive(json, "reference_height");
		hud.right_margin = json.number("right_margin");
		hud.ammo_top_margin = json.number("ammo_top_margin");
		hud.health_top_margin = json.number("health_top_margin");
		hud.box_size = read_vector(json, "box_size");
		hud.box_fill_border = json.number("box_fill_border");
		hud.health_colour = read_colour(json, "health_colour");

		hud.sprite_sheet = json.string("sprite_sheet");
		hud.fill_frame = json.string("fill_frame");

		const JsonValue timer = json.object("timer");
		hud.timer_font = timer.string("font");
		hud.timer_offset = read_vector(timer, "offset");
		hud.timer_shadow_offset = read_vector(timer, "shadow_offset");
		hud.timer_colour = read_colour(timer, "colour");
		hud.timer_shadow_colour = read_colour(timer, "shadow_colour");
		hud.timer_reference_width = read_positive(timer, "reference_width");

		const JsonValue identity = json.object("player_identity");
		hud.player_identity_font = identity.string("font");
		hud.player_identity_offset = read_vector(identity, "offset");
		hud.player_identity_shadow_offset = read_vector(identity, "shadow_offset");
		hud.player_identity_colour = read_colour(identity, "colour");
		hud.player_identity_shadow_colour = read_colour(identity, "shadow_colour");
		hud.player_identity_background_colour = read_colour(identity, "background_colour");
		hud.player_identity_padding = read_vector(identity, "padding");
		hud.player_identity_reference_width = read_positive(identity, "reference_width");

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
	this->source_path_ = path;
	const JsonValue root = document.root();

	this->menu_ = read_menu_style(root);
	this->main_menu_ = read_main_menu(root);
	const JsonValue controls = root.object("controls_settings");
	this->controls_settings_.layout = read_layout(controls, "layout");
	this->controls_settings_.heading_position = read_vector(controls, "heading_position");
	this->controls_settings_.controls_position = read_vector(controls, "controls_position");
	this->controls_settings_.help_font = controls.string("help_font");
	this->pause_menu_ = read_pause_menu(root);
	this->end_menu_ = read_end_menu(root);
	this->results_menu_ = read_results_menu(root);
	this->prompt_bar_ = read_prompt_bar(root);
	this->hud_ = read_hud(root);
	this->camera_.reference_height = read_positive(root.object("camera"), "reference_height");
	const JsonValue match = root.object("match");
	this->match_.reference_size = read_positive_vector(match, "reference_size");
	this->match_.countdown_size = read_positive_vector(match, "countdown_size");
	this->match_.countdown_center_y = read_positive(match, "countdown_center_y");
	if (this->match_.countdown_center_y >= 1.0f)
	{
		throw std::runtime_error(match.where() + ": countdown_center_y must be less than one.");
	}
	this->match_.controls_margin = read_positive_vector(match, "controls_margin");
	this->match_.controls_padding = read_positive_vector(match, "controls_padding");
	this->match_.controls_gap = read_positive(match, "controls_gap");
	const MatchPresentation& intro = this->match_;
	if (intro.countdown_size.x <= intro.controls_padding.x * 2.0f ||
		intro.countdown_size.y <= intro.controls_padding.y * 3.0f ||
		intro.countdown_size.x > intro.reference_size.x ||
		intro.countdown_size.y / 2.0f > intro.reference_size.y *
			std::min(intro.countdown_center_y, 1.0f - intro.countdown_center_y) ||
		intro.controls_margin.x * 2.0f + intro.controls_gap +
			intro.controls_padding.x * 4.0f >= intro.reference_size.x ||
		intro.controls_margin.y * 2.0f + intro.controls_padding.y * 3.0f >=
			intro.reference_size.y)
	{
		throw std::runtime_error(match.where() +
			": countdown and controls cards must fit the reference size with room inside their padding.");
	}
	this->debug_overlay_ = read_debug_overlay(root);
}

const MatchPresentation& Presentation::match() const
{
	return this->match_;
}

const CameraPresentation& Presentation::camera() const
{
	return this->camera_;
}

void Presentation::validate_resources(const RenderResources& render,
	const AudioResources& audio) const
{
	const ResourceValidation check(this->source_path_, render, audio);
	check.font("menu_style.fonts.heading", this->menu_.heading_font);
	check.font("menu_style.fonts.item", this->menu_.item_font);
	check.font("menu_style.fonts.detail", this->menu_.detail_font);
	check.font("menu_style.chrome.brand_font", this->menu_.brand_font);
	check.frame("main_menu.backdrop.frame", this->main_menu_.backdrop_sheet,
		this->main_menu_.backdrop_frame);
	check.font("main_menu.fonts.title", this->main_menu_.title_font);
	check.font("main_menu.fonts.announcement", this->main_menu_.announcement_font);
	check.font("main_menu.fonts.weapon_description", this->main_menu_.weapon_description_font);
	check.font("controls_settings.help_font", this->controls_settings_.help_font);
	check.font("results_menu.contribution_font", this->results_menu_.contribution_font);
	check.font("prompt_bar.font", this->prompt_bar_.font);
	check.font("debug_overlay.font", this->debug_overlay_.font);
	check.font("hud.timer.font", this->hud_.timer_font);
	check.font("hud.meter_label_font", this->hud_.meter_label_font);
	check.font("hud.player_identity.font", this->hud_.player_identity_font);
	check.font("hud.respawn_timer.font", this->hud_.respawn_font);
	check.frame("hud.fill_frame", this->hud_.sprite_sheet, this->hud_.fill_frame);
	const Vector2F heading_size = render.measure_text(
		render.resolve_sprite_font(this->menu_.detail_font), L"GET READY");
	const float heading_scale = std::min(1.0f,
		(this->match_.countdown_size.x - this->match_.controls_padding.x * 2.0f) /
			(heading_size.x + std::abs(this->menu_.detail_shadow_offset.x)));
	if ((heading_size.y + std::abs(this->menu_.detail_shadow_offset.y)) * heading_scale +
		this->match_.controls_padding.y * 3.0f >= this->match_.countdown_size.y)
	{
		throw std::runtime_error(this->source_path_ +
			": match.countdown_size must leave room below the GET READY heading.");
	}

	const std::string& bank = this->menu_.sound_bank;
	check.wave("menu_style.sounds.direction", bank, this->menu_.direction_sound);
	check.wave("menu_style.sounds.confirm", bank, this->menu_.confirm_sound);
	check.wave("menu_style.sounds.cancel", bank, this->menu_.cancel_sound);
	check.wave("main_menu.sounds.error", bank, this->main_menu_.error_sound);
	check.wave("main_menu.sounds.ready", bank, this->main_menu_.ready_sound);
	check.effect("main_menu.music.name", bank, this->main_menu_.music.name);
	check.wave("pause_menu.window_open_sound", bank, this->pause_menu_.window_open_sound);
	check.wave("results_menu.sounds.winner.name", bank, this->results_menu_.winner_sound.name);
	check.effect("results_menu.sounds.fill.name", bank, this->results_menu_.fill_sound.name);

	// These menu pictures are chosen by the page code, so they have no JSON
	// field to walk. Check them too before the player can reach those pages.
	for (const char* frame : { "pixel", "square_white_4", "team_select_a",
		"team_select_center", "team_select_b", "team_select_a_selected",
		"team_select_center_selected", "team_select_b_selected", "results_menu_fill_box",
		"results_menu_team_a_fill", "results_menu_team_b_fill" })
	{
		check.frame(std::string("built-in menu frame '") + frame + "'",
			"sprite_sheet_1", frame);
	}
	for (const char* frame : { "pixel", "sprayer", "sniper", "roller",
		"mister", "bucket", "random", "error" })
	{
		check.frame(std::string("built-in weapon menu frame '") + frame + "'",
			"weapon_paint", frame);
	}
	const SpriteSheet* weapon_sheet = render.sprite_sheet("weapon_paint");
	for (const char* frame : { "sprayer", "sniper", "roller", "mister", "bucket" })
	{
		const std::string paint_name = std::string(frame) + "_paint";
		check.frame("built-in weapon menu paint overlay", "weapon_paint", paint_name);
		const SpriteFrame& body = weapon_sheet->sprite_frame(
			weapon_sheet->resolve_sprite_frame(frame));
		const SpriteFrame& paint = weapon_sheet->sprite_frame(
			weapon_sheet->resolve_sprite_frame(paint_name));
		if (body.source_rectangle().size() != paint.source_rectangle().size() ||
			body.origin() != paint.origin())
		{
			throw std::runtime_error(this->source_path_ + ": built-in weapon menu frame '" +
				frame + "' paint overlay must match its dimensions and origin.");
		}
	}
}

const MenuStyle& Presentation::menu() const
{
	return this->menu_;
}

const MainMenuPresentation& Presentation::main_menu() const
{
	return this->main_menu_;
}

const ControlsSettingsPresentation& Presentation::controls_settings() const
{
	return this->controls_settings_;
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

const PromptBarPresentation& Presentation::prompt_bar() const
{
	return this->prompt_bar_;
}

const HudPresentation& Presentation::hud() const
{
	return this->hud_;
}

const DebugOverlayPresentation& Presentation::debug_overlay() const
{
	return this->debug_overlay_;
}
