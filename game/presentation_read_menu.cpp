#include "game/pch.h"
#include "game/presentation_read.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/objects/player_badge.h"
#include "game/resource_validation.h"
#include "game/states/ui_player_badge.h"
#include "game/text_wrap.h"
#include "engine/render/sprite_geometry.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

// Readers for the menu-page sections of presentation.json: team palette,
// menu style, main menu, controls settings, pause, end, results and prompt bar.

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;

namespace
{
	PresentationSound read_sound(const JsonValue& parent, const char* key)
	{
		const JsonValue sound = parent.object(key);

		PresentationSound result;
		result.name = sound.identifier("name");
		if (result.name.empty())
		{
			throw std::runtime_error(sound.where() + ": 'name' must not be empty.");
		}
		result.volume = read_unit(sound, "volume");
		return result;
	}

	WidgetLayout read_layout(const JsonValue& parent, const char* key)
	{
		const JsonValue json = parent.object(key);

		WidgetLayout layout;
		layout.position = read_vector_unchecked(json, "position");
		layout.size = read_vector_unchecked(json, "size");
		layout.spacing = read_vector_unchecked(json, "spacing");
		return layout;
	}

	SelectPresentation read_select(const JsonValue& parent, const char* key)
	{
		const JsonValue json = parent.object(key);

		SelectPresentation select;
		select.spacing = read_vector_unchecked(json, "spacing");
		select.size = read_vector_unchecked(json, "size");
		select.selected = read_colour(json, "selected");
		select.unselected = read_colour(json, "unselected");
		return select;
	}
}

namespace presentation_read
{
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
		const JsonValue layout = json.object("layout");
		style.layout.position = read_vector(layout, "position");
		style.layout.size = read_positive_vector(layout, "size");
		style.layout.spacing = read_vector(layout, "spacing");
		if (style.layout.position.x < 0.0f || style.layout.position.y < 0.0f ||
			style.layout.position.x + style.layout.size.x > 1920.0f ||
			style.layout.position.y + style.layout.size.y > 1080.0f)
		{
			throw std::runtime_error(layout.where() +
				": 'position' and 'size' must place the first widget inside the (1920,1080) menu design.");
		}
		if (style.layout.spacing.x < 0.0f || style.layout.spacing.y < 0.0f ||
			style.layout.spacing.x > 1920.0f || style.layout.spacing.y > 1080.0f)
		{
			throw std::runtime_error(layout.where() +
				": 'spacing' must be nonnegative and at most (1920,1080).");
		}
		style.sprite_sheet = json.identifier("sprite_sheet");
		style.fill_frame = json.identifier("fill_frame");
		if (style.sprite_sheet.empty() || style.fill_frame.empty())
		{
			throw std::runtime_error(json.where() + ": 'sprite_sheet' and 'fill_frame' must not be empty.");
		}

		const JsonValue navigation = json.object("navigation");
		style.repeat_delay = read_positive(navigation, "repeat_delay");
		style.repeat_interval = read_positive(navigation, "repeat_interval");
		const JsonValue motion = json.object("motion");
		style.motion.focus_duration = read_positive(motion, "focus_duration");
		style.motion.press_duration = read_positive(motion, "press_duration");
		// Travel is in keyline units.
		style.motion.arrow_travel = read_number_in(motion, "arrow_travel", 0.0f, 2.0f);
		style.motion.press_fraction = read_unit(motion, "press_fraction");
		style.motion.selection_inset_fraction = read_number_in(motion, "selection_inset_fraction", 0.0f, 0.08f);
		if (style.motion.focus_duration > 0.5f || style.motion.press_duration > 0.5f)
		{
			throw std::runtime_error(motion.where() + ": focus_duration and press_duration must be at most 0.5 seconds.");
		}

		const JsonValue fonts = json.object("fonts");
		style.heading_font = fonts.string("heading");
		style.item_font = fonts.string("item");
		style.detail_font = fonts.string("detail");

		const JsonValue shadow = json.object("shadow");
		style.shadow_colour = read_colour(shadow, "colour");
		style.title_shadow_offset = read_vector_unchecked(shadow, "title_offset");
		style.heading_shadow_offset = read_vector_unchecked(shadow, "heading_offset");
		style.item_shadow_offset = read_vector_unchecked(shadow, "item_offset");
		style.detail_shadow_offset = read_vector_unchecked(shadow, "detail_offset");

		style.highlight = read_colour(json, "highlight");
		style.unhighlight = read_colour(json, "unhighlight");
		style.disabled = read_colour(json, "disabled");
		style.warning = read_colour(json, "warning");
		style.heading_colour = read_colour(json, "heading_colour");
		const JsonValue chrome = json.object("chrome");
		style.focus_panel_colour = read_colour(chrome, "focus_panel_colour");
		style.focus_padding = read_vector_unchecked(chrome, "focus_padding");
		style.accent_width = read_positive(chrome, "accent_width");
		style.panel_edge_colour = read_colour(chrome, "panel_edge_colour");
		style.panel_shadow_offset = read_vector_unchecked(chrome, "panel_shadow_offset");
		style.scrim_colour = read_colour(chrome, "scrim_colour");
		style.brand_position = read_vector_unchecked(chrome, "brand_position");
		style.brand_font = chrome.string("brand_font");
		style.footer_y = chrome.number("footer_y");
		style.rule_height = read_positive(chrome, "rule_height");
		style.icon_padding = read_vector_unchecked(chrome, "icon_padding");
		style.box_size = read_vector_unchecked(json, "box_size");

		style.sound_bank = json.string("sound_bank");

		const JsonValue sounds = json.object("sounds");
		style.direction_sound = read_sound(sounds, "direction");
		style.confirm_sound = read_sound(sounds, "confirm");
		style.cancel_sound = read_sound(sounds, "cancel");

		return style;
	}
}

namespace
{
	MenuPaperArtPresentation read_menu_paper(const JsonValue& json, const Vector2F& box_size);
}

namespace presentation_read
{
	MainMenuPresentation read_main_menu(const JsonValue& root, float content_height)
	{
		const JsonValue json = root.object("main_menu");
		if (!std::isfinite(content_height) || content_height <= 40.0f || content_height > 1080.0f)
		{
			throw std::runtime_error(json.where() +
				": menu_style.chrome.footer_y must be finite, greater than 40 and at most 1080.");
		}

		MainMenuPresentation menu;
		const auto panel_size = [&](const char* key, float max_width = 1760.0f)
		{
			const Vector2F size = read_positive_vector(json, key);
			if (size.x > max_width || size.y > content_height - 40.0f)
			{
				throw std::runtime_error(json.where() + ": '" + key +
					"' must be at most " + std::to_string(static_cast<int>(max_width)) +
					" wide and leave 20 pixels above and below within menu_style.chrome.footer_y.");
			}
			return size;
		};
		menu.title_box_size = panel_size("title_box_size");
		menu.home_box_size = panel_size("home_box_size");
		menu.options_box_size = panel_size("options_box_size");
		menu.credits_box_size = panel_size("credits_box_size");
		menu.setup_box_size = panel_size("setup_box_size");
		menu.team_box_size = panel_size("team_box_size", 1840.0f);
		menu.weapon_box_size = panel_size("weapon_box_size", 1840.0f);
		menu.stage_box_size = panel_size("stage_box_size", 1840.0f);
		menu.art = read_menu_paper(json, Vector2F(
			std::min({ menu.title_box_size.x, menu.home_box_size.x,
				menu.options_box_size.x, menu.credits_box_size.x, menu.setup_box_size.x, menu.team_box_size.x,
				menu.weapon_box_size.x, menu.stage_box_size.x }),
			std::min({ menu.title_box_size.y, menu.home_box_size.y,
				menu.options_box_size.y, menu.credits_box_size.y, menu.setup_box_size.y, menu.team_box_size.y,
				menu.weapon_box_size.y, menu.stage_box_size.y })));
		const JsonValue art = json.object("art");
		menu.art.disabled_ink = read_colour(art, "disabled_ink");
		menu.setup_focus_padding = read_positive_vector(json, "setup_focus_padding");
		menu.setup_shadow_offset = read_number_in(json, "setup_shadow_offset", 0.0f, 24.0f);
		if (menu.setup_focus_padding.x > 48.0f ||
			menu.setup_focus_padding.y > 24.0f ||
			menu.setup_focus_padding.x <= menu.setup_shadow_offset + menu.art.keyline_width ||
			menu.setup_focus_padding.y <= menu.setup_shadow_offset + menu.art.keyline_width)
		{
			throw std::runtime_error(json.where() +
				": setup_focus_padding must be at most (48,24) "
				"and exceed setup_shadow_offset + art.keyline_width on both axes.");
		}
		if (menu.art.disabled_ink.a != 1.0f || menu.art.disabled_ink == menu.art.paper ||
			menu.art.disabled_ink == menu.art.focus_fill || menu.art.disabled_ink == menu.art.ink ||
			menu.art.disabled_ink == menu.art.focus_ink)
		{
			throw std::runtime_error(art.where() + ": disabled_ink must be opaque and distinct from paper and active ink.");
		}
		const auto position = [&](const char* key)
		{
			const Vector2F value = read_vector(json, key);
			if (value.x < 0.0f || value.x > 1920.0f || value.y < 0.0f || value.y > 1080.0f)
			{
				throw std::runtime_error(json.where() + ": '" + key +
					"' must be nonnegative and inside the (1920,1080) menu design.");
			}
			return value;
		};
		// A height in the (1920,1080) menu design.
		const auto title_height = [&](const char* key)
		{
			return read_number_in(json, key, 0.0f, 1080.0f);
		};
		const JsonValue backdrop = json.object("backdrop");
		menu.backdrop_sheet = backdrop.string("sheet");
		menu.backdrop_frame = backdrop.string("frame");
		menu.title_y = title_height("title_y");
		menu.title_subtitle_y = title_height("title_subtitle_y");
		menu.title_start_y = title_height("title_start_y");
		menu.title_no_pad_y = title_height("title_no_pad_y");
		menu.home_position = position("home_position");
		menu.home_subtitle_position = position("home_subtitle_position");

		const JsonValue fonts = json.object("fonts");
		menu.title_font = fonts.string("title");
		menu.announcement_font = fonts.string("announcement");
		menu.weapon_description_font = fonts.string("weapon_description");
		menu.build_id_y = title_height("build_id_y");

		const JsonValue backgrounds = json.object("backgrounds");
		menu.title_background_colour = read_colour(backgrounds, "title");
		menu.home_background_colour = read_colour(backgrounds, "home");
		menu.options_background_colour = read_colour(backgrounds, "options");
		menu.play_background_colour = read_colour(backgrounds, "play");

		menu.options_value_colour = read_colour(json, "options_value_colour");
		menu.options_warning_colour = read_colour(json, "options_warning_colour");
		if (menu.options_warning_colour.a != 1.0f || menu.options_warning_colour == menu.art.paper ||
			menu.options_warning_colour == menu.art.focus_fill)
		{
			throw std::runtime_error(json.where() +
				": options_warning_colour must be opaque and distinct from paper and focus_fill.");
		}
		menu.options_heading_position = position("options_heading_position");
		menu.options_position = position("options_position");
		menu.options_status_position = position("options_status_position");
		menu.options_value_x = json.number("options_value_x");
		menu.options_spacing = read_positive_vector(json, "options_spacing");
		menu.display_trial_seconds = read_number_in(json, "display_trial_seconds", 1.0f, 60.0f);
		menu.home_spacing = read_positive_vector(json, "home_spacing");
		MenuPaperArtPresentation options_art = menu.art;
		options_art.focus_padding = menu.setup_focus_padding;
		options_art.shadow_offset = menu.setup_shadow_offset;
		const float edge = menu.art.keyline_width * 3.0f;
		const float title_top = (content_height - menu.title_box_size.y) / 2.0f + edge;
		const float title_bottom = (content_height + menu.title_box_size.y) / 2.0f -
			menu.art.shadow_offset - edge;
		if (menu.title_y <= title_top ||
			menu.title_subtitle_y <= menu.title_y ||
			menu.title_start_y <= menu.title_subtitle_y ||
			menu.title_no_pad_y <= menu.title_start_y ||
			menu.build_id_y <= menu.title_no_pad_y ||
			menu.build_id_y >= title_bottom)
		{
			throw std::runtime_error(json.where() +
				": title_y < title_subtitle_y < title_start_y < "
				"title_no_pad_y < build_id_y must fit vertically inside title_box_size.");
		}
		const Vector2F home_top = (Vector2F(1920.0f, content_height) - menu.home_box_size) / 2.0f +
			Vector2F(edge);
		const Vector2F home_bottom = (Vector2F(1920.0f, content_height) + menu.home_box_size) / 2.0f -
			Vector2F(menu.art.shadow_offset + edge);
		if (menu.home_position.x <= home_top.x || menu.home_position.x >= home_bottom.x ||
			menu.home_subtitle_position.x <= home_top.x || menu.home_subtitle_position.x >= home_bottom.x ||
			menu.home_position.y <= home_top.y ||
			menu.home_subtitle_position.y <= menu.home_position.y ||
			menu.home_subtitle_position.y >= menu.home_position.y + menu.home_spacing.y * 2.0f ||
			menu.home_position.y + menu.home_spacing.y * 5.0f >= home_bottom.y ||
			menu.home_spacing.x > 1920.0f)
		{
			throw std::runtime_error(json.where() +
				": home_position, home_subtitle_position and home_spacing must place the heading, "
				"subtitle and five row intervals inside home_box_size.");
		}

		const JsonValue credits = json.object("credits");
		menu.credits_font = credits.identifier("font");
		menu.credits_heading_position = read_vector(credits, "heading_position");
		menu.credits_position = read_vector(credits, "position");
		menu.credits_back_position = read_vector(credits, "back_position");
		menu.credits_text = drawable_text::widen_drawable(credits.string("text"),
			credits.where() + ": text");

		const auto inside_panel = [&](const Vector2F& point, const Vector2F& size,
			const char* key, const JsonValue& owner, float shadow_offset)
		{
			const Vector2F top = (Vector2F(1920.0f, content_height) - size) / 2.0f + Vector2F(edge);
			const Vector2F bottom = (Vector2F(1920.0f, content_height) + size) / 2.0f -
				Vector2F(shadow_offset + edge);
			if (point.x <= top.x || point.x >= bottom.x || point.y <= top.y || point.y >= bottom.y)
			{
				throw std::runtime_error(owner.where() + ": '" + key + "' must fit inside its box_size.");
			}
		};
		inside_panel(menu.options_heading_position, menu.options_box_size, "options_heading_position", json, options_art.shadow_offset);
		inside_panel(menu.options_position, menu.options_box_size, "options_position", json, options_art.shadow_offset);
		inside_panel(menu.options_status_position, menu.options_box_size, "options_status_position", json, options_art.shadow_offset);
		if (!std::isfinite(menu.options_value_x) ||
			menu.options_value_x <= menu.options_position.x + options_art.focus_padding.x ||
			menu.options_value_x >= (1920.0f + menu.options_box_size.x) / 2.0f -
				options_art.shadow_offset - options_art.focus_padding.x)
		{
			throw std::runtime_error(json.where() + ": options_value_x must follow the label inside options_box_size.");
		}
		if (menu.options_heading_position.y >= menu.options_position.y ||
			menu.options_position.y + menu.options_spacing.y * 6.0f >= menu.options_status_position.y ||
			menu.options_spacing.y <= options_art.focus_padding.y * 2.0f ||
			menu.options_spacing.x > 1920.0f)
		{
			throw std::runtime_error(json.where() + ": options_heading_position, options_position, options_spacing and "
				"options_status_position must order the heading, seven rows and status inside options_box_size.");
		}
		inside_panel(menu.credits_heading_position, menu.credits_box_size, "heading_position", credits, menu.art.shadow_offset);
		inside_panel(menu.credits_position, menu.credits_box_size, "position", credits, menu.art.shadow_offset);
		inside_panel(menu.credits_back_position, menu.credits_box_size, "back_position", credits, menu.art.shadow_offset);
		if (menu.credits_heading_position.y >= menu.credits_position.y ||
			menu.credits_position.y >= menu.credits_back_position.y)
		{
			throw std::runtime_error(credits.where() + ": heading_position, position and back_position must be in vertical order.");
		}

		const JsonValue description = json.object("weapon_description");
		menu.weapon_description_colour = read_colour(description, "colour");
		menu.weapon_description_shadow_offset =
			read_vector_unchecked(description, "shadow_offset");
		menu.weapon_description_x_offset = read_finite(description, "x_offset");
		menu.weapon_description_max_width = read_positive(description, "max_width");

		menu.mode_select_spacing = read_vector_unchecked(json, "mode_select_spacing");
		menu.player_count_spacing = read_vector_unchecked(json, "player_count_spacing");

		menu.team_select = read_select(json, "team_select");
		const JsonValue team_select = json.object("team_select");
		menu.team_select.spacing = read_positive_vector(team_select, "spacing");
		menu.team_select.size = read_positive_vector(team_select, "size");
		menu.team_warning_position = read_vector(team_select, "warning_position");
		if (menu.team_warning_position.x < 0.0f || menu.team_warning_position.x > 1920.0f ||
			menu.team_warning_position.y < 0.0f || menu.team_warning_position.y > content_height)
		{
			throw std::runtime_error(team_select.where() +
				": warning_position must be nonnegative and inside the menu design above menu_style.chrome.footer_y.");
		}
		menu.team_select_sheet = team_select.identifier("sheet");
		if (menu.team_select_sheet.empty())
		{
			throw std::runtime_error(team_select.where() + ": 'sheet' must not be empty.");
		}
		const JsonValue frames = team_select.object("frames");
		const JsonValue selected_frames = team_select.object("selected_frames");
		const char* roles[] = { "a", "center", "b" };
		for (size_t index = 0; index < std::size(roles); ++index)
		{
			menu.team_select_frames[index] = frames.identifier(roles[index]);
			menu.team_select_selected_frames[index] = selected_frames.identifier(roles[index]);
			if (menu.team_select_frames[index].empty() || menu.team_select_selected_frames[index].empty())
			{
				throw std::runtime_error(team_select.where() + ": 'frames' and 'selected_frames' role '" +
					roles[index] + "' must not be empty.");
			}
		}
		menu.weapon_select = read_select(json, "weapon_select");
		const JsonValue weapon_select = json.object("weapon_select");
		menu.weapon_select.spacing = read_positive_vector(weapon_select, "spacing");
		menu.weapon_select.size = read_positive_vector(weapon_select, "size");
		WeaponSelectArtworkPresentation& artwork = menu.weapon_select_art;
		artwork.sheet = weapon_select.identifier("sheet");
		artwork.fill_frame = weapon_select.identifier("fill_frame");
		if (artwork.sheet.empty() || artwork.fill_frame.empty())
		{
			throw std::runtime_error(weapon_select.where() + ": 'sheet' and 'fill_frame' must not be empty.");
		}
		const JsonValue choices = weapon_select.object("choices");
		for (size_t index = 0; index < weapon_choice_keys.size(); ++index)
		{
			const JsonValue choice = choices.object(weapon_choice_keys[index]);
			WeaponChoicePresentation& record = artwork.choices[index];
			record.name = drawable_text::widen_drawable(choice.string("name"), choice.where() + ": name");
			if (record.name.find_first_not_of(L' ') == std::wstring::npos ||
				record.name.find_first_of(L"\r\n") != std::wstring::npos)
			{
				throw std::runtime_error(choice.where() + ": 'name' must be nonblank and single-line.");
			}
			record.frame = choice.identifier("frame");
			record.paint_frame = choice.identifier("paint_frame");
			record.supply_strip = choice.identifier("supply_strip");
			if (record.frame.empty())
			{
				throw std::runtime_error(choice.where() + ": 'frame' must not be empty.");
			}
		}
		const JsonValue stage = json.object("stage_select");
		menu.stage_select.preview_position = read_vector_unchecked(stage, "preview_position");
		menu.stage_select.preview_size = read_positive_vector(stage, "preview_size");
		menu.stage_select.name_position = read_vector_unchecked(stage, "name_position");
		menu.stage_select.navigation_position = read_vector_unchecked(stage, "navigation_position");
		menu.stage_select.record_y = read_number_in(stage, "record_y", 0.0f, 1080.0f);
		menu.stage_select.record_font = stage.string("record_font");
		menu.stage_select.briefing = read_layout(stage, "briefing");
		menu.stage_select.briefing.size = read_positive_vector(stage.object("briefing"), "size");
		menu.stage_select.briefing_padding = read_positive_vector(stage, "briefing_padding");
		menu.stage_select.selected = read_colour(stage, "selected");
		menu.stage_select.unselected = read_colour(stage, "unselected");
		menu.stage_select.selected_ink = read_colour(stage, "selected_ink");

		menu.music = read_sound(json, "music");

		const JsonValue sounds = json.object("sounds");
		menu.error_sound = read_sound(sounds, "error");
		menu.ready_sound = read_sound(sounds, "ready");
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
}

namespace
{
	MenuPaperArtPresentation read_menu_paper(const JsonValue& json, const Vector2F& box_size)
	{
		MenuPaperArtPresentation drawing;
		const JsonValue art = json.object("art");
		drawing.ink = read_colour(art, "ink");
		drawing.paper = read_colour(art, "paper");
		drawing.shade = read_colour(art, "shade");
		drawing.focus_fill = read_colour(art, "focus_fill");
		drawing.focus_ink = read_colour(art, "focus_ink");
		drawing.keyline_width = read_positive(art, "keyline_width");
		drawing.shadow_offset = art.number("shadow_offset");
		drawing.focus_padding = read_positive_vector(art, "focus_padding");
		if (drawing.keyline_width > 8.0f ||
			!std::isfinite(drawing.shadow_offset) || drawing.shadow_offset < 0.0f ||
			drawing.shadow_offset > 24.0f || drawing.focus_padding.x > 48.0f ||
			drawing.focus_padding.y > 24.0f)
		{
			throw std::runtime_error(art.where() +
				": keyline_width <= 8, shadow_offset 0..24 and focus_padding <= (48,24) required.");
		}
		if (drawing.focus_padding.x <= drawing.shadow_offset + drawing.keyline_width ||
			drawing.focus_padding.y <= drawing.shadow_offset + drawing.keyline_width)
		{
			throw std::runtime_error(art.where() +
				": focus_padding must exceed shadow_offset + keyline_width on both axes.");
		}
		if (box_size.x <= drawing.shadow_offset + drawing.keyline_width * 6.0f ||
			box_size.y <= drawing.shadow_offset + drawing.keyline_width * 6.0f)
		{
			throw std::runtime_error(json.where() +
				": box_size must exceed art.shadow_offset + art.keyline_width * 6 on both axes.");
		}
		if (drawing.ink.a != 1.0f || drawing.paper.a != 1.0f ||
			drawing.focus_fill.a != 1.0f || drawing.focus_ink.a != 1.0f ||
			drawing.shade.a <= 0.0f || drawing.ink == drawing.paper ||
			drawing.focus_ink == drawing.focus_fill || drawing.ink == drawing.focus_fill ||
			drawing.focus_fill == drawing.paper)
		{
			throw std::runtime_error(art.where() +
				": opaque distinct paper/ink and focus colours, and visible shade required.");
		}
		return drawing;
	}
}

namespace presentation_read
{
	ControlsSettingsPresentation read_controls_settings(const JsonValue& root, float content_height)
	{
		const JsonValue json = root.object("controls_settings");
		ControlsSettingsPresentation menu;
		menu.box_size = read_positive_vector(json, "box_size");
		if (menu.box_size.x > 1760.0f || menu.box_size.y > content_height - 40.0f)
		{
			throw std::runtime_error(json.where() +
				": box_size must be at most 1760 wide and leave 20 pixels above and below within menu_style.chrome.footer_y.");
		}
		menu.art = read_menu_paper(json, menu.box_size);
		menu.heading_position = read_vector(json, "heading_position");
		menu.controls_position = read_vector(json, "controls_position");
		menu.controls_size = read_positive_vector(json, "controls_size");
		menu.status_position = read_vector(json, "status_position");
		const JsonValue layout = json.object("layout");
		menu.layout.position = read_vector(layout, "position");
		menu.layout.size = read_positive_vector(layout, "size");
		menu.layout.spacing = read_positive_vector(layout, "spacing");
		menu.item_font = json.identifier("item_font");
		menu.help_font = json.identifier("help_font");
		menu.volume_preview_sound = read_sound(json.object("sounds"), "volume_preview");
		const float edge = menu.art.keyline_width * 3.0f;
		const Vector2F top = (Vector2F(1920.0f, content_height) - menu.box_size) / 2.0f + Vector2F(edge);
		const Vector2F bottom = (Vector2F(1920.0f, content_height) + menu.box_size) / 2.0f -
			Vector2F(menu.art.shadow_offset + edge);
		const auto inside_panel = [&](const Vector2F& point, const char* key)
		{
			if (point.x <= top.x || point.x >= bottom.x || point.y <= top.y || point.y >= bottom.y)
			{
				throw std::runtime_error(json.where() + ": '" + key + "' must fit inside box_size.");
			}
		};
		inside_panel(menu.heading_position, "heading_position");
		inside_panel(menu.controls_position, "controls_position");
		inside_panel(menu.status_position, "status_position");
		if (menu.status_position.x >= (1920.0f + menu.box_size.x) / 2.0f -
			menu.art.shadow_offset - menu.art.focus_padding.x)
		{
			throw std::runtime_error(json.where() + ": status_position must leave positive text width inside box_size and art.focus_padding.");
		}
		inside_panel(menu.layout.position - menu.art.focus_padding, "layout.position");
		const Vector2F help_bottom = menu.controls_position + menu.controls_size;
		if (help_bottom.x >= bottom.x || help_bottom.y >= bottom.y)
		{
			throw std::runtime_error(json.where() + ": controls_position and controls_size must fit inside box_size.");
		}

		// Six settings, Keyboard Jump/Pad Buttons and Apply/Back share the left column. Reserve their
		// authored size, focus card and shadow before the help and status text.
		const Vector2F last_row_bottom = menu.layout.position + menu.layout.size +
			Vector2F(menu.layout.spacing.x, menu.layout.spacing.y * 9.0f);
		const Vector2F rows_bottom = last_row_bottom + menu.art.focus_padding + Vector2F(menu.art.shadow_offset);
		if (rows_bottom.x >= menu.controls_position.x || rows_bottom.y >= bottom.y ||
			menu.layout.spacing.x <= menu.layout.size.x + menu.art.focus_padding.x ||
			menu.layout.spacing.y < menu.layout.size.y + menu.art.focus_padding.y * 2.0f + menu.art.shadow_offset)
		{
			throw std::runtime_error(json.where() + ": layout.position, layout.size and layout.spacing must keep ten rows "
				"and their focus cards inside box_size and before controls_position.");
		}
		if (menu.heading_position.y >= menu.layout.position.y - menu.art.focus_padding.y ||
			menu.heading_position.y >= menu.controls_position.y ||
			menu.status_position.y <= rows_bottom.y || menu.status_position.x >= menu.controls_position.x)
		{
			throw std::runtime_error(json.where() + ": heading_position, controls_position and status_position must order "
				"the heading, ten rows and status without overlapping the help column.");
		}

		// Validate the shared page layout first so its diagnostics still identify
		// a malformed controls column before the preview placed inside it.
		const JsonValue preview = json.object("stick_preview");
		const auto read_preview_box = [&](const char* key)
		{
			const JsonValue box = preview.object(key);
			return RectangleF(read_vector(box, "position"), read_positive_vector(box, "size"));
		};
		menu.stick_preview.raw_movement = read_preview_box("raw_movement");
		menu.stick_preview.movement_response = read_preview_box("movement_response");
		menu.stick_preview.aim = read_preview_box("aim");
		menu.stick_preview.aim_readout_position = read_vector(preview, "aim_readout_position");
		menu.stick_preview.association_position = read_vector(preview, "association_position");
		menu.stick_preview.marker_size = read_positive(preview, "marker_size");
		menu.stick_preview.line_width = read_positive(preview, "line_width");
		const StickPreviewPresentation& view = menu.stick_preview;
		if (view.marker_size > 24.0f || view.line_width > 8.0f ||
			view.marker_size <= view.line_width * 2.0f)
		{
			throw std::runtime_error(preview.where() + ": marker_size must be <=24 and greater than twice line_width; line_width must be <=8.");
		}
		const RectangleF preview_bounds(menu.controls_position, menu.controls_size);
		const auto check_preview_box = [&](const char* key, const RectangleF& box)
		{
			if (!preview_bounds.contains(box) || box.width <= view.marker_size * 2.0f ||
				box.height <= view.line_width * 2.0f)
			{
				throw std::runtime_error(preview.where() + ": '" + key +
					"' must fit controls_size and leave space for marker_size and line_width.");
			}
		};
		check_preview_box("raw_movement", view.raw_movement);
		check_preview_box("movement_response", view.movement_response);
		check_preview_box("aim", view.aim);
		if (view.aim.width != view.aim.height)
		{
			throw std::runtime_error(preview.where() + ": aim must be square.");
		}
		const auto check_preview_position = [&](const char* key, const Vector2F& position)
		{
			if (position.x < preview_bounds.x || position.x >= preview_bounds.right() ||
				position.y < preview_bounds.y || position.y >= preview_bounds.bottom())
			{
				throw std::runtime_error(preview.where() + ": '" + key + "' must fit inside controls_size.");
			}
		};
		check_preview_position("aim_readout_position", view.aim_readout_position);
		check_preview_position("association_position", view.association_position);
		if (view.raw_movement.y <= preview_bounds.y ||
			view.movement_response.y <= view.raw_movement.bottom() ||
			view.aim.y <= view.movement_response.bottom() ||
			view.association_position.y <= view.aim.bottom())
		{
			throw std::runtime_error(preview.where() +
				": raw_movement, movement_response, aim and association_position must be ordered with gaps for their labels.");
		}
		if (view.aim_readout_position.x <= view.aim.right() ||
			view.aim_readout_position.y < view.aim.y || view.aim_readout_position.y >= view.aim.bottom())
		{
			throw std::runtime_error(preview.where() + ": aim_readout_position must sit beside the aim box.");
		}
		return menu;
	}

	PauseMenuPresentation read_pause_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("pause_menu");

		PauseMenuPresentation menu;
		menu.resume_countdown = read_positive(json, "resume_countdown");
		if (menu.resume_countdown > 3.0f)
		{
			throw std::runtime_error(json.where() + ": 'resume_countdown' must be at most 3 seconds.");
		}
		menu.box_size = read_positive_vector(json, "box_size");
		if (menu.box_size.x > 1600.0f || menu.box_size.y > 900.0f)
		{
			throw std::runtime_error(json.where() + ": box_size must be at most (1600,900).");
		}
		menu.art = read_menu_paper(json, menu.box_size);
		const JsonValue art = json.object("art");
		menu.art.disabled_ink = read_colour(art, "disabled_ink");
		if (menu.art.disabled_ink.a != 1.0f || menu.art.disabled_ink == menu.art.paper ||
			menu.art.disabled_ink == menu.art.focus_fill || menu.art.disabled_ink == menu.art.ink ||
			menu.art.disabled_ink == menu.art.focus_ink)
		{
			throw std::runtime_error(art.where() + ": disabled_ink must be opaque and distinct from paper and active ink.");
		}
		menu.heading_y = read_positive(json, "heading_y");
		menu.status_y = read_positive(json, "status_y");
		if (menu.heading_y > 900.0f || menu.status_y > 900.0f || menu.status_y <= menu.heading_y)
		{
			throw std::runtime_error(json.where() + ": heading_y < status_y <= 900 required.");
		}
		menu.layout = read_layout(json, "layout");
		menu.window_open_sound = read_sound(json, "window_open_sound");
		const JsonValue sounds = json.object("sounds");
		menu.pad_lost_sound = read_sound(sounds, "pad_lost");
		menu.pad_back_sound = read_sound(sounds, "pad_back");
		menu.resume_sound = read_sound(sounds, "resume");
		return menu;
	}

	EndMenuPresentation read_end_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("end_menu");

		EndMenuPresentation menu;
		menu.box_size = read_positive_vector(json, "box_size");
		menu.heading_y = read_positive(json, "heading_y");
		if (menu.heading_y > 900.0f)
		{
			throw std::runtime_error(json.where() + ": heading_y must be at most 900.");
		}
		if (menu.box_size.x > 1600.0f || menu.box_size.y > 900.0f)
		{
			throw std::runtime_error(json.where() + ": box_size must be at most (1600,900).");
		}
		menu.art = read_menu_paper(json, menu.box_size);
		menu.layout = read_layout(json, "layout");
		menu.series_sound = read_sound(json.object("sounds"), "series");
		return menu;
	}

	ResultsMenuPresentation read_results_menu(const JsonValue& root)
	{
		const JsonValue json = root.object("results_menu");

		ResultsMenuPresentation menu;
		menu.box_size = read_positive_vector(json, "box_size");
		if (menu.box_size.x > 1600.0f || menu.box_size.y > 900.0f)
		{
			throw std::runtime_error(json.where() + ": box_size must be at most (1600,900).");
		}
		menu.art = read_menu_paper(json, menu.box_size);
		const auto read_y = [&](const char* key)
		{
			const float value = read_positive(json, key);
			if (value > 900.0f)
			{
				throw std::runtime_error(json.where() + ": " + key + " must be at most 900.");
			}
			return value;
		};
		menu.heading_y = read_y("heading_y");
		menu.winner_y = read_y("winner_y");
		menu.session_score_y = read_y("session_score_y");
		menu.session_result_y = read_y("session_result_y");
		menu.proceed_y = read_y("proceed_y");
		menu.record_y = read_y("record_y");
		if (menu.heading_y >= menu.winner_y || menu.winner_y >= menu.session_score_y ||
			menu.session_score_y >= menu.session_result_y)
		{
			throw std::runtime_error(json.where() +
				": heading_y < winner_y < session_score_y < session_result_y required.");
		}
		menu.contribution_position = read_vector_unchecked(json, "contribution_position");
		menu.contribution_spacing = read_positive(json, "contribution_spacing");
		if (menu.contribution_spacing > 100.0f)
		{
			throw std::runtime_error(json.where() + ": contribution_spacing must be at most 100.");
		}
		menu.contribution_font = json.string("contribution_font");

		const JsonValue fill = json.object("fill");
		menu.fill_box_size = read_positive_vector(fill, "box_size");
		menu.fill_box_y_offset = read_number_in(fill, "box_y_offset", 0.0f, 900.0f);
		menu.team_fill_border = read_positive_vector(fill, "border");
		menu.track_colour = read_colour(fill, "track_colour");
		if (menu.track_colour.a != 1.0f || menu.track_colour == menu.art.ink)
		{
			throw std::runtime_error(fill.where() + ": track_colour must be opaque and distinct from art.ink.");
		}
		if (menu.fill_box_size.x > menu.box_size.x ||
			menu.fill_box_size.y > menu.box_size.y || menu.team_fill_border.x < menu.art.keyline_width ||
			menu.team_fill_border.y < menu.art.keyline_width ||
			menu.team_fill_border.x * 2.0f >= menu.fill_box_size.x ||
			menu.team_fill_border.y * 2.0f >= menu.fill_box_size.y)
		{
			throw std::runtime_error(fill.where() +
				": box_size must fit the panel, and border must contain the keyline and leave positive fill space.");
		}
		menu.team_fill_delay = read_number_in(fill, "delay", 0.0f, 10.0f);
		// Zero has no defined fill ratio. Reject it here instead of sending a
		// first-frame NaN to the renderer.
		menu.team_fill_time = read_positive(fill, "time");

		// Derive the fill size from the authored box and border.
		menu.team_fill_size =
			menu.fill_box_size - menu.team_fill_border * 2.0f;

		menu.show_results_delay = read_number_in(json, "show_results_delay", 0.0f, 10.0f);
		if (menu.team_fill_time > 10.0f)
		{
			throw std::runtime_error(fill.where() + ": 'time' must be at most 10.");
		}

		const JsonValue team_mark = json.object("team_mark");
		menu.team_mark_size = read_positive_vector(team_mark, "size");
		menu.team_mark_gap = read_number_in(team_mark, "gap", 0.0f, 100.0f);

		const JsonValue sounds = json.object("sounds");
		menu.winner_sound = read_sound(sounds, "winner");
		menu.draw_sound = read_sound(sounds, "draw");
		menu.fill_sound = read_sound(sounds, "fill");
		menu.music = read_sound(json, "music");

		return menu;
	}

	PromptBarPresentation read_prompt_bar(const JsonValue& root)
	{
		const JsonValue json = root.object("prompt_bar");

		PromptBarPresentation bar;
		bar.font = json.string("font");
		bar.position = read_vector_unchecked(json, "position");
		bar.colour = read_colour(json, "colour");
		bar.shadow_colour = read_colour(json, "shadow_colour");
		bar.shadow_offset = read_vector_unchecked(json, "shadow_offset");
		return bar;
	}
}
