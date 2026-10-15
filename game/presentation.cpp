#include "game/pch.h"
#include "game/presentation.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "game/drawable_text.h"
#include "game/resource_validation.h"

#include <cmath>
#include <initializer_list>
#include <limits>
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

	MovementBurstPresentation read_movement_burst(const JsonValue& parent,
		const char* key)
	{
		const JsonValue json = parent.object(key);
		return { read_positive(json, "duration"), read_positive(json, "spread"),
			read_positive(json, "lift"), read_positive(json, "particle_size") };
	}

	Vector2F read_finite_vector(const JsonValue& parent, const char* key)
	{
		const Vector2F value = read_vector(parent, key);
		if (!std::isfinite(value.x) || !std::isfinite(value.y))
		{
			throw std::runtime_error(parent.where() + ": '" + key + "' must be finite.");
		}
		return value;
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

	Colour colour_from_text(const JsonValue& where, const std::string& text);

	// Accept named colours or RRGGBB/RRGGBBAA. Validate hex strings before
	// construction so a typo cannot silently become opaque black.
	Colour read_colour(const JsonValue& parent, const char* key)
	{
		return colour_from_text(parent, parent.string(key));
	}

	std::vector<Colour> read_colours(const JsonValue& parent, const char* key)
	{
		const JsonValue array = parent.array(key);
		if (array.size() == 0)
		{
			throw std::runtime_error(array.where() + ": '" + key + "' needs a colour.");
		}
		std::vector<Colour> colours;
		for (size_t index = 0; index < array.size(); ++index)
		{
			colours.push_back(colour_from_text(array.at(index), array.at(index).as_string()));
		}
		return colours;
	}

	Colour colour_from_text(const JsonValue& parent, const std::string& text)
	{

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
		result.name = sound.identifier("name");
		if (result.name.empty())
		{
			throw std::runtime_error(sound.where() + ": 'name' must not be empty.");
		}
		result.volume = sound.number("volume");
		if (!std::isfinite(result.volume) || result.volume < 0.0f || result.volume > 1.0f)
		{
			throw std::runtime_error(sound.where() + ": 'volume' must be finite and from 0 to 1.");
		}
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

	TeamPalettePresentation read_team_palette(const JsonValue& root)
	{
		const JsonValue json = root.object("team_palette");
		TeamPalettePresentation palette;
		const JsonValue colours = json.array("colours");
		for (size_t index = 0; index < colours.size(); ++index)
		{
			const JsonValue entry = colours.at(index);
			const std::string name = entry.identifier("name");
			if (name.empty()) { throw std::runtime_error(entry.where() + ": team colour name must not be empty."); }
			for (const NamedTeamColour& existing : palette.colours)
			{
				if (existing.name == name)
				{
					throw std::runtime_error(entry.where() + ": duplicate team colour '" + name + "'.");
				}
			}
			palette.colours.push_back({ name, read_colour(entry, "colour") });
		}
		const JsonValue pairs = json.array("pairs");
		for (size_t index = 0; index < pairs.size(); ++index)
		{
			const JsonValue entry = pairs.at(index);
			const std::string key = entry.identifier("key");
			if (key.empty()) { throw std::runtime_error(entry.where() + ": team pair key must not be empty."); }
			if (key == "random")
			{
				throw std::runtime_error(entry.where() + ": 'random' is a preference, not a fixed pair.");
			}
			for (const TeamPaletteEntry& existing : palette.pairs)
			{
				if (existing.key == key)
				{
					throw std::runtime_error(entry.where() + ": duplicate team pair '" + key + "'.");
				}
			}
			const auto resolve = [&](const char* field) -> Colour
			{
				const std::string name = entry.identifier(field);
				for (const NamedTeamColour& colour : palette.colours)
				{
					if (colour.name == name) { return colour.colour; }
				}
				throw std::runtime_error(entry.where() + ": '" + field + "' names unknown team colour '" + name + "'.");
			};
			const std::wstring label = drawable_text::widen_drawable(entry.identifier("label"), entry.where() + ": label");
			if (label.empty())
			{
				throw std::runtime_error(entry.where() + ": team pair label must not be empty.");
			}
			palette.pairs.push_back({ key, label, { resolve("team_a"), resolve("team_b") } });
		}
		// Saved fixed preferences must always resolve, regardless of array order.
		for (int value = 0; value < static_cast<int>(TeamColourPair::count); ++value)
		{
			const char* key = TeamColourTools::palette_key(static_cast<TeamColourPair>(value));
			bool found = false;
			for (const TeamPaletteEntry& pair : palette.pairs) { found |= pair.key == key; }
			if (!found) { throw std::runtime_error(pairs.where() + ": missing fixed team pair '" + key + "'."); }
		}
		return palette;
	}

	MenuStyle read_menu_style(const JsonValue& root)
	{
		const JsonValue json = root.object("menu_style");

		MenuStyle style;

		const JsonValue navigation = json.object("navigation");
		style.repeat_delay = read_positive(navigation, "repeat_delay");
		style.repeat_interval = read_positive(navigation, "repeat_interval");

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
		menu.home_spacing = read_positive_vector(json, "home_spacing");

		const JsonValue credits = json.object("credits");
		menu.credits_font = credits.identifier("font");
		menu.credits_position = read_finite_vector(credits, "position");
		menu.credits_back_position = read_finite_vector(credits, "back_position");
		menu.credits_text = drawable_text::widen_drawable(credits.string("text"),
			credits.where() + ": text");

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
		menu.unready_sound = read_sound(sounds, "unready");

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
		const JsonValue sounds = json.object("sounds");
		menu.pad_lost_sound = read_sound(sounds, "pad_lost");
		menu.pad_back_sound = read_sound(sounds, "pad_back");
		return menu;
	}

	EndMenuPresentation read_end_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("end_menu");

		EndMenuPresentation menu;
		menu.box_colour = read_colour(json, "box_colour");
		menu.layout = read_layout(json, "layout");
		menu.series_sound = read_sound(json.object("sounds"), "series");
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
		menu.draw_sound = read_sound(sounds, "draw");
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


		return hud;
	}

	ToolMotionPresentation read_tool_motion(const JsonValue& root)
	{
		const JsonValue json = root.object("tool_motion");
		ToolMotionPresentation motion;
		motion.enabled = json.boolean("enabled");
		motion.shot_pose_split = read_positive(json, "shot_pose_split");
		motion.roller_travel_per_turn = read_positive(json, "roller_travel_per_turn");
		motion.roller_lift_time = read_positive(json, "roller_lift_time");
		motion.roller_settle_time = read_positive(json, "roller_settle_time");
		motion.hose_travel_per_sway = read_positive(json, "hose_travel_per_sway");
		motion.hose_settle_time = read_positive(json, "hose_settle_time");
		motion.bucket_travel_per_swing = read_positive(json, "bucket_travel_per_swing");
		motion.bucket_settle_time = read_positive(json, "bucket_settle_time");
		const JsonValue tools = json.object("tools");
		const std::array<const char*, 5> names = { "sprayer", "sniper", "roller", "mister", "bucket" };
		for (size_t index = 0; index < names.size(); ++index)
		{
			const JsonValue tool = tools.object(names[index]);
			motion.tools[index] = { read_positive(tool, "shot_recovery"),
				read_positive(tool, "valve_time") };
		}
		const JsonValue mist = json.object("mist_tail");
		motion.mist_sheet = mist.identifier("sprite_sheet");
		motion.mist_frame = mist.identifier("fill_frame");
		motion.mist_tail_duration = read_positive(mist, "duration");
		motion.mist_tail_distance = read_positive(mist, "distance");
		motion.mist_tail_size = read_positive_vector(mist, "size");
		motion.mist_tail_opacity = read_positive(mist, "opacity");
		if (motion.shot_pose_split >= 1.0f || motion.mist_tail_opacity > 1.0f)
		{
			throw std::runtime_error(json.where() +
				": shot_pose_split must be less than one and mist opacity at most one.");
		}
		return motion;
	}

	FigurePresentation read_figure(const JsonValue& root)
	{
		const JsonValue json = root.object("figure");
		FigurePresentation figure;
		figure.sprite_sheet = json.identifier("sprite_sheet");
		const JsonValue accents = json.array("accent_frames");
		if (accents.size() != figure.accent_frames.size())
		{
			throw std::runtime_error(accents.where() + ": needs four distinct teammate accent frames.");
		}
		for (size_t index = 0; index < figure.accent_frames.size(); ++index)
		{
			figure.accent_frames[index] = accents.at(index).as_identifier();
			for (size_t previous = 0; previous < index; ++previous)
			{
				if (figure.accent_frames[index] == figure.accent_frames[previous])
				{
					throw std::runtime_error(accents.at(index).where() + ": teammate accent frames must be distinct.");
				}
			}
		}
		figure.rest_arms_frame = json.identifier("rest_arms_frame");
		figure.canvas_size = read_positive_vector(json, "canvas_size");
		figure.accent_offset = read_finite_vector(json, "accent_offset");
		figure.accent_size = read_positive_vector(json, "accent_size");
		figure.rear_shoulder = read_finite_vector(json, "rear_shoulder");
		figure.front_shoulder = read_finite_vector(json, "front_shoulder");
		figure.arm_width = read_positive(json, "arm_width");
		figure.elbow_drop = read_positive(json, "elbow_drop");
		figure.hand_size = read_positive(json, "hand_size");
		figure.hand_edge = read_positive(json, "hand_edge");
		figure.arm_colour = read_colour(json, "arm_colour");
		figure.hand_colour = read_colour(json, "hand_colour");
		if (figure.hand_edge * 2.0f >= figure.hand_size)
		{
			throw std::runtime_error(json.where() + ": 'hand_edge' must leave an interior in 'hand_size'.");
		}
		const JsonValue grips = json.object("grips");
		const std::array<const char*, 5> keys = { "sprayer", "sniper", "roller", "mister", "bucket" };
		for (size_t index = 0; index < keys.size(); ++index)
		{
			const JsonValue grip = grips.object(keys[index]);
			figure.grips[index] = { read_finite_vector(grip, "rear"), read_finite_vector(grip, "front") };
		}
		return figure;
	}

	RespawnRedrawPresentation read_respawn_redraw(const JsonValue& root)
	{
		const JsonValue json = root.object("respawn_redraw");

		RespawnRedrawPresentation redraw;
		redraw.sprite_sheet = json.identifier("sprite_sheet");
		redraw.pen_frame = json.identifier("pen_frame");
		redraw.pen_colour = read_colour(json, "pen_colour");
		redraw.canvas_size = read_positive_vector(json, "canvas_size");
		redraw.pen_width = read_positive(json, "pen_width");
		redraw.draw_from = json.number("draw_from");
		redraw.fill_at = json.number("fill_at");
		if (!(redraw.draw_from >= 0.0f && redraw.draw_from < redraw.fill_at &&
			redraw.fill_at <= 1.0f))
		{
			throw std::runtime_error(json.where() + ": 'draw_from' and 'fill_at' must be "
				"fractions of the respawn wait with 0 <= draw_from < fill_at <= 1.");
		}
		redraw.camera_hold = json.number("camera_hold");
		if (!std::isfinite(redraw.camera_hold) || redraw.camera_hold < 0.0f)
		{
			throw std::runtime_error(json.where() + ": 'camera_hold' must be finite and "
				"not negative.");
		}

		const JsonValue strokes = json.array("strokes");
		if (strokes.size() == 0)
		{
			throw std::runtime_error(strokes.where() + ": a figure needs at least one stroke.");
		}
		const Vector2F last_nib = redraw.canvas_size - Vector2F(redraw.pen_width);
		for (size_t index = 0; index < strokes.size(); ++index)
		{
			const JsonValue stroke_json = strokes.at(index);
			RespawnStroke stroke;
			stroke.part = stroke_json.string("part");
			const JsonValue points = stroke_json.array("points");
			if (points.size() < 2)
			{
				throw std::runtime_error(points.where() + ": a stroke needs at least two points.");
			}
			for (size_t at = 0; at < points.size(); ++at)
			{
				const JsonValue point_json = points.at(at);
				const Vector2F point(point_json.number("x"), point_json.number("y"));
				// The pen's whole nib stays on the canvas.
				if (!(point.x >= 0.0f && point.y >= 0.0f &&
					point.x <= last_nib.x && point.y <= last_nib.y))
				{
					throw std::runtime_error(point_json.where() +
						": the pen leaves the canvas at this point.");
				}
				if (!stroke.points.empty())
				{
					const Vector2F step = point - stroke.points.back();
					if ((step.x == 0.0f) == (step.y == 0.0f))
					{
						throw std::runtime_error(point_json.where() + ": the pen draws only "
							"horizontal and vertical lines, and never stands still.");
					}
				}
				stroke.points.push_back(point);
			}
			redraw.strokes.push_back(std::move(stroke));
		}
		return redraw;
	}

	float read_fraction(const JsonValue& parent, const char* key)
	{
		const float value = parent.number(key);
		if (!(value >= 0.0f && value <= 1.0f))
		{
			throw std::runtime_error(parent.where() + ": '" + key +
				"' must be a fraction from 0 to 1.");
		}
		return value;
	}

	OpeningPresentation read_opening(const JsonValue& root)
	{
		const JsonValue json = root.object("opening");

		OpeningPresentation opening;
		opening.sprite_sheet = json.identifier("sprite_sheet");
		opening.fill_frame = json.identifier("fill_frame");
		opening.bezel_shades = read_colours(json, "bezel_shades");
		opening.bezel_shadow = read_colour(json, "bezel_shadow");
		const JsonValue glass = json.object("glass");
		opening.glass = RectangleF(read_fraction(glass, "x"), read_fraction(glass, "y"),
			read_fraction(glass, "width"), read_fraction(glass, "height"));
		if (opening.glass.width <= 0.0f || opening.glass.height <= 0.0f ||
			opening.glass.right() > 1.0f || opening.glass.bottom() > 1.0f)
		{
			throw std::runtime_error(glass.where() + ": the glass must be on the screen.");
		}
		opening.glass_colour = read_colour(json, "glass_colour");
		opening.program_colour = read_colour(json, "program_colour");
		opening.desktop_colour = read_colour(json, "desktop_colour");
		opening.palette_height = read_fraction(json, "palette_height");
		opening.palette_greys = read_colours(json, "palette_greys");
		for (const Colour& grey : opening.palette_greys)
		{
			if (grey.r != grey.g || grey.g != grey.b)
			{
				throw std::runtime_error(json.where() +
					": every colour in 'palette_greys' must be a grey.");
			}
		}
		opening.slot_outline = read_colour(json, "slot_outline");
		opening.sticky_notes = read_colours(json, "sticky_notes");
		opening.stickers = read_colours(json, "stickers");
		opening.scribble = read_colour(json, "scribble");
		opening.coffee = read_colour(json, "coffee");
		opening.crack = read_colour(json, "crack");
		opening.silhouette = read_colour(json, "silhouette");
		opening.adult_height = read_fraction(json, "adult_height");
		opening.child_height = read_fraction(json, "child_height");
		opening.cursor_size = read_fraction(json, "cursor_size");
		opening.preview_strength = read_fraction(json, "preview_strength");
		opening.figure_grey = read_colour(json, "figure_grey");
		opening.percent_font = json.identifier("percent_font");
		opening.percent_height = read_fraction(json, "percent_height");
		opening.percent_shadow = read_colour(json, "percent_shadow");
		const JsonValue acting = json.object("acting");
		opening.face = read_colour(acting, "face");
		opening.sweat = read_colour(acting, "sweat");
		opening.hop = read_fraction(acting, "hop");
		opening.shake = read_fraction(acting, "shake");
		opening.slump = read_fraction(acting, "slump");
		if (opening.adult_height <= 0.0f || opening.child_height <= 0.0f ||
			opening.cursor_size <= 0.0f || opening.palette_height <= 0.0f ||
			opening.percent_height <= 0.0f)
		{
			throw std::runtime_error(json.where() + ": 'adult_height', 'child_height', "
				"'cursor_size', 'palette_height' and 'percent_height' must be more than zero.");
		}
		return opening;
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

const TeamPaletteEntry& TeamPalettePresentation::fixed_pair(TeamColourPair pair) const
{
	const char* key = TeamColourTools::palette_key(pair);
	for (const TeamPaletteEntry& entry : this->pairs)
	{
		if (entry.key == key) { return entry; }
	}
	throw std::invalid_argument("Missing fixed team palette");
}

const TeamPaletteEntry& TeamPalettePresentation::random_pair(size_t draw) const
{
	if (this->pairs.empty()) { throw std::invalid_argument("No team palette pairs to draw"); }
	return this->pairs[draw % this->pairs.size()];
}

const TeamPalettePresentation& Presentation::team_palette() const
{
	return this->team_palette_;
}

Presentation::Presentation(const char* path)
{
	const JsonDocument document = read_json_file(path);
	this->source_path_ = path;
	const JsonValue root = document.root();

	this->team_palette_ = read_team_palette(root);
	this->menu_ = read_menu_style(root);
	this->main_menu_ = read_main_menu(root);
	const JsonValue controls = root.object("controls_settings");
	this->controls_settings_.layout = read_layout(controls, "layout");
	this->controls_settings_.heading_position = read_vector(controls, "heading_position");
	this->controls_settings_.controls_position = read_vector(controls, "controls_position");
	this->controls_settings_.help_font = controls.string("help_font");
	this->controls_settings_.volume_preview_sound = read_sound(controls.object("sounds"), "volume_preview");
	this->pause_menu_ = read_pause_menu(root);
	this->end_menu_ = read_end_menu(root);
	this->results_menu_ = read_results_menu(root);
	this->prompt_bar_ = read_prompt_bar(root);
	this->hud_ = read_hud(root);
	this->camera_.reference_height = read_positive(root.object("camera"), "reference_height");
	const JsonValue movement = root.object("movement_feedback");
	MovementFeedbackPresentation& feedback = this->movement_feedback_;
	feedback.enabled = movement.boolean("enabled");
	feedback.sprite_sheet = movement.string("sprite_sheet");
	feedback.fill_frame = movement.string("fill_frame");
	feedback.colour = read_colour(movement, "colour");
	feedback.highlight = read_colour(movement, "highlight");
	feedback.pixel_size = read_positive(movement, "pixel_size");
	feedback.landing_min_speed = read_positive(movement, "landing_min_speed");
	feedback.full_impact_speed = read_positive(movement, "full_impact_speed");
	feedback.minimum_strength = read_positive(movement, "minimum_strength");
	feedback.takeoff = read_movement_burst(movement, "takeoff");
	feedback.landing = read_movement_burst(movement, "landing");
	if (feedback.minimum_strength > 1.0f ||
		feedback.full_impact_speed < feedback.landing_min_speed ||
		feedback.takeoff.particle_size < feedback.pixel_size ||
		feedback.landing.particle_size < feedback.pixel_size)
	{
		throw std::runtime_error(movement.where() +
			": strength must be at most one, full impact speed must cover minimum speed, "
			"and particles must be at least one presentation pixel.");
	}
	this->figure_ = read_figure(root);
	this->tool_motion_ = read_tool_motion(root);
	this->respawn_redraw_ = read_respawn_redraw(root);
	this->opening_ = read_opening(root);
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

const MovementFeedbackPresentation& Presentation::movement_feedback() const
{
	return this->movement_feedback_;
}

const RespawnRedrawPresentation& Presentation::respawn_redraw() const
{
	return this->respawn_redraw_;
}

const FigurePresentation& Presentation::figure() const
{
	return this->figure_;
}

const ToolMotionPresentation& Presentation::tool_motion() const
{
	return this->tool_motion_;
}

const OpeningPresentation& Presentation::opening() const
{
	return this->opening_;
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
	check.font("main_menu.credits.font", this->main_menu_.credits_font);
	check.font("controls_settings.help_font", this->controls_settings_.help_font);
	check.font("results_menu.contribution_font", this->results_menu_.contribution_font);
	check.font("prompt_bar.font", this->prompt_bar_.font);
	check.font("debug_overlay.font", this->debug_overlay_.font);
	check.font("hud.timer.font", this->hud_.timer_font);
	check.font("hud.meter_label_font", this->hud_.meter_label_font);
	check.font("hud.player_identity.font", this->hud_.player_identity_font);
	check.frame("hud.fill_frame", this->hud_.sprite_sheet, this->hud_.fill_frame);
	check.frame("movement_feedback.fill_frame", this->movement_feedback_.sprite_sheet,
		this->movement_feedback_.fill_frame);
	for (size_t index = 0; index < this->figure_.accent_frames.size(); ++index)
	{
		check.frame("figure.accent_frames[" + std::to_string(index) + "]", this->figure_.sprite_sheet,
			this->figure_.accent_frames[index]);
	}
	check.frame("figure.rest_arms_frame", this->figure_.sprite_sheet, this->figure_.rest_arms_frame);
	check.frame("tool_motion.mist_tail.fill_frame", this->tool_motion_.mist_sheet,
		this->tool_motion_.mist_frame);
	check.frame("respawn_redraw.pen_frame", this->respawn_redraw_.sprite_sheet,
		this->respawn_redraw_.pen_frame);
	check.frame("opening.fill_frame", this->opening_.sprite_sheet,
		this->opening_.fill_frame);
	check.font("opening.percent_font", this->opening_.percent_font);
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
	check.wave("main_menu.sounds.unready.name", bank, this->main_menu_.unready_sound.name);
	check.wave("controls_settings.sounds.volume_preview.name", bank, this->controls_settings_.volume_preview_sound.name);
	check.effect("main_menu.music.name", bank, this->main_menu_.music.name);
	check.wave("pause_menu.window_open_sound", bank, this->pause_menu_.window_open_sound);
	check.wave("pause_menu.sounds.pad_lost.name", bank, this->pause_menu_.pad_lost_sound.name);
	check.wave("pause_menu.sounds.pad_back.name", bank, this->pause_menu_.pad_back_sound.name);
	check.wave("end_menu.sounds.series.name", bank, this->end_menu_.series_sound.name);
	check.wave("results_menu.sounds.winner.name", bank, this->results_menu_.winner_sound.name);
	check.wave("results_menu.sounds.draw.name", bank, this->results_menu_.draw_sound.name);
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

void Presentation::set_movement_feedback_enabled(bool enabled)
{
	this->movement_feedback_.enabled = enabled;
}
