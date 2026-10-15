#include "game/pch.h"
#include "game/presentation.h"
#include "game/build_info.h"

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
	constexpr std::array<const char*, WeaponSelectArtworkPresentation::CHOICES> weapon_choice_keys{
		"sprayer", "sniper", "roller", "mister", "bucket", "random"
	};
	constexpr std::array<const char*, static_cast<size_t>(MenuHeading::count)> menu_heading_keys{
		"home", "options", "credits", "controls_audio", "pad_sticks", "players", "join_players",
		"mode", "teams", "weapons", "arena"
	};
	constexpr std::array<const char*, static_cast<size_t>(MenuPrompt::count)> menu_prompt_keys{
		"title", "select_back", "options", "controls", "sticks", "mode", "join", "team", "team_shuffle",
		"weapon", "arena_browse", "arena_locked", "missing_player", "pause", "pause_missing", "select", "credits"
	};
	constexpr std::array<const char*, static_cast<size_t>(MenuItem::count)> menu_item_keys{
		"play", "options", "credits", "exit", "resolution", "fullscreen", "team_palette",
		"controls_audio", "apply", "back", "single_match", "best_of_three", "best_of_five",
		"match_length", "one_player_practice", "two_players", "three_players", "four_players",
		"resume", "restart", "quit", "yes", "no", "change_teams", "change_weapons", "change_level",
		"exit_to_main_menu", "pad_sticks", "music_volume", "sfx_volume", "view_shake", "aim_marker",
		"reduce_effects", "pad", "move_deadzone", "aim_deadzone"
	};

	std::wstring read_menu_line(const JsonValue& json, const char* key)
	{
		std::wstring text = drawable_text::widen_drawable(json.string(key), json.where() + ": " + key);
		if (text.find_first_not_of(L' ') == std::wstring::npos ||
			text.find_first_of(L"\r\n") != std::wstring::npos)
		{
			throw std::runtime_error(json.where() + ": '" + key + "' must be nonblank and single-line.");
		}
		return text;
	}

	std::wstring read_menu_wordmark(const JsonValue& json)
	{
		std::wstring text = drawable_text::widen_drawable(json.string("wordmark"), json.where() + ": wordmark");
		if (text.find(L'\r') != std::wstring::npos)
		{
			throw std::runtime_error(json.where() + ": 'wordmark' must use LF for authored line breaks.");
		}
		size_t begin = 0;
		do
		{
			const size_t end = text.find(L'\n', begin);
			if (text.substr(begin, end == std::wstring::npos ? end : end - begin).find_first_not_of(L' ') ==
				std::wstring::npos)
			{
				throw std::runtime_error(json.where() + ": 'wordmark' must have a nonblank line between each authored LF break.");
			}
			if (end == std::wstring::npos) { break; }
			begin = end + 1;
		} while (begin <= text.size());
		return text;
	}

	MenuText read_menu_text(const JsonValue& root)
	{
		const JsonValue json = root.object("menu_text");
		MenuText text;
		text.brand = read_menu_line(json, "brand");
		text.solo_practice_hint = read_menu_line(json, "solo_practice_hint");
		const JsonValue title = json.object("title");
		text.title.wordmark = read_menu_wordmark(title);
		text.title.subtitle = read_menu_line(title, "subtitle");
		text.title.start = read_menu_line(title, "start");
		text.title.no_controller = read_menu_line(title, "no_controller");
		text.home_subtitle = read_menu_line(json, "home_subtitle");
		const JsonValue headings = json.object("headings");
		for (size_t index = 0; index < text.headings.size(); ++index)
		{
			text.headings[index] = read_menu_line(headings, menu_heading_keys[index]);
		}
		const JsonValue prompts = json.object("prompts");
		for (size_t index = 0; index < text.prompts.size(); ++index)
		{
			text.prompts[index] = read_menu_line(prompts, menu_prompt_keys[index]);
		}
		const JsonValue items = json.object("items");
		for (size_t index = 0; index < text.items.size(); ++index)
		{
			text.items[index] = read_menu_line(items, menu_item_keys[index]);
		}
		return text;
	}

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
		const JsonValue layout = json.object("layout");
		style.layout.position = read_finite_vector(layout, "position");
		style.layout.size = read_positive_vector(layout, "size");
		style.layout.spacing = read_finite_vector(layout, "spacing");
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

	MenuPaperArtPresentation read_menu_paper(const JsonValue& json, const Vector2F& box_size);

	MainMenuPresentation read_main_menu(const JsonValue& root, float content_height)
	{
		const JsonValue json = root.object("main_menu");
		if (!std::isfinite(content_height) || content_height <= 40.0f || content_height > 1080.0f)
		{
			throw std::runtime_error(json.where() +
				": menu_style.chrome.footer_y must be finite, greater than 40 and at most 1080.");
		}

		MainMenuPresentation menu;
		const auto panel_size = [&](const char* key)
		{
			const Vector2F size = read_positive_vector(json, key);
			if (size.x > 1760.0f || size.y > content_height - 40.0f)
			{
				throw std::runtime_error(json.where() + ": '" + key +
					"' must be at most 1760 wide and leave 20 pixels above and below within menu_style.chrome.footer_y.");
			}
			return size;
		};
		menu.title_box_size = panel_size("title_box_size");
		menu.home_box_size = panel_size("home_box_size");
		menu.options_box_size = panel_size("options_box_size");
		menu.credits_box_size = panel_size("credits_box_size");
		menu.art = read_menu_paper(json, Vector2F(
			std::min({ menu.title_box_size.x, menu.home_box_size.x,
				menu.options_box_size.x, menu.credits_box_size.x }),
			std::min({ menu.title_box_size.y, menu.home_box_size.y,
				menu.options_box_size.y, menu.credits_box_size.y })));
		const JsonValue art = json.object("art");
		menu.art.disabled_ink = read_colour(art, "disabled_ink");
		if (menu.art.disabled_ink.a != 1.0f || menu.art.disabled_ink == menu.art.paper ||
			menu.art.disabled_ink == menu.art.focus_fill || menu.art.disabled_ink == menu.art.ink ||
			menu.art.disabled_ink == menu.art.focus_ink)
		{
			throw std::runtime_error(art.where() + ": disabled_ink must be opaque and distinct from paper and active ink.");
		}
		const auto position = [&](const char* key)
		{
			const Vector2F value = read_finite_vector(json, key);
			if (value.x < 0.0f || value.x > 1920.0f || value.y < 0.0f || value.y > 1080.0f)
			{
				throw std::runtime_error(json.where() + ": '" + key +
					"' must be nonnegative and inside the (1920,1080) menu design.");
			}
			return value;
		};
		const auto title_height = [&](const char* key)
		{
			const float value = json.number(key);
			if (!std::isfinite(value) || value < 0.0f || value > 1080.0f)
			{
				throw std::runtime_error(json.where() + ": '" + key +
					"' must be finite and from 0 to 1080 in the menu design.");
			}
			return value;
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
		menu.home_spacing = read_positive_vector(json, "home_spacing");
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
		menu.credits_heading_position = read_finite_vector(credits, "heading_position");
		menu.credits_position = read_finite_vector(credits, "position");
		menu.credits_back_position = read_finite_vector(credits, "back_position");
		menu.credits_text = drawable_text::widen_drawable(credits.string("text"),
			credits.where() + ": text");

		const auto inside_panel = [&](const Vector2F& point, const Vector2F& size,
			const char* key, const JsonValue& owner)
		{
			const Vector2F top = (Vector2F(1920.0f, content_height) - size) / 2.0f + Vector2F(edge);
			const Vector2F bottom = (Vector2F(1920.0f, content_height) + size) / 2.0f -
				Vector2F(menu.art.shadow_offset + edge);
			if (point.x <= top.x || point.x >= bottom.x || point.y <= top.y || point.y >= bottom.y)
			{
				throw std::runtime_error(owner.where() + ": '" + key + "' must fit inside its box_size.");
			}
		};
		inside_panel(menu.options_heading_position, menu.options_box_size, "options_heading_position", json);
		inside_panel(menu.options_position, menu.options_box_size, "options_position", json);
		inside_panel(menu.options_status_position, menu.options_box_size, "options_status_position", json);
		if (!std::isfinite(menu.options_value_x) ||
			menu.options_value_x <= menu.options_position.x + menu.art.focus_padding.x ||
			menu.options_value_x >= (1920.0f + menu.options_box_size.x) / 2.0f -
				menu.art.shadow_offset - menu.art.focus_padding.x)
		{
			throw std::runtime_error(json.where() + ": options_value_x must follow the label inside options_box_size.");
		}
		if (menu.options_heading_position.y >= menu.options_position.y ||
			menu.options_position.y + menu.options_spacing.y * 5.0f >= menu.options_status_position.y ||
			menu.options_spacing.y <= menu.art.focus_padding.y * 2.0f ||
			menu.options_spacing.x > 1920.0f)
		{
			throw std::runtime_error(json.where() + ": options_heading_position, options_position, options_spacing and "
				"options_status_position must order the heading, six rows and status inside options_box_size.");
		}
		inside_panel(menu.credits_heading_position, menu.credits_box_size, "heading_position", credits);
		inside_panel(menu.credits_position, menu.credits_box_size, "position", credits);
		inside_panel(menu.credits_back_position, menu.credits_box_size, "back_position", credits);
		if (menu.credits_heading_position.y >= menu.credits_position.y ||
			menu.credits_position.y >= menu.credits_back_position.y)
		{
			throw std::runtime_error(credits.where() + ": heading_position, position and back_position must be in vertical order.");
		}

		const JsonValue description = json.object("weapon_description");
		menu.weapon_description_colour = read_colour(description, "colour");
		menu.weapon_description_shadow_offset =
			read_vector(description, "shadow_offset");
		menu.weapon_description_x_offset = description.number("x_offset");

		menu.mode_select_spacing = read_vector(json, "mode_select_spacing");
		menu.player_count_spacing = read_vector(json, "player_count_spacing");

		menu.team_select = read_select(json, "team_select");
		const JsonValue team_select = json.object("team_select");
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
		menu.heading_position = read_finite_vector(json, "heading_position");
		menu.controls_position = read_finite_vector(json, "controls_position");
		menu.controls_size = read_positive_vector(json, "controls_size");
		menu.status_position = read_finite_vector(json, "status_position");
		const JsonValue layout = json.object("layout");
		menu.layout.position = read_finite_vector(layout, "position");
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

		// Six settings and Apply/Back share the left column. Reserve their
		// authored size, focus card and shadow before the help and status text.
		const Vector2F last_row_bottom = menu.layout.position + menu.layout.size +
			Vector2F(menu.layout.spacing.x, menu.layout.spacing.y * 7.0f);
		const Vector2F rows_bottom = last_row_bottom + menu.art.focus_padding + Vector2F(menu.art.shadow_offset);
		if (rows_bottom.x >= menu.controls_position.x || rows_bottom.y >= bottom.y ||
			menu.layout.spacing.x <= menu.layout.size.x + menu.art.focus_padding.x ||
			menu.layout.spacing.y < menu.layout.size.y + menu.art.focus_padding.y * 2.0f + menu.art.shadow_offset)
		{
			throw std::runtime_error(json.where() + ": layout.position, layout.size and layout.spacing must keep eight rows "
				"and their focus cards inside box_size and before controls_position.");
		}
		if (menu.heading_position.y >= menu.layout.position.y - menu.art.focus_padding.y ||
			menu.heading_position.y >= menu.controls_position.y ||
			menu.status_position.y <= rows_bottom.y || menu.status_position.x >= menu.controls_position.x)
		{
			throw std::runtime_error(json.where() + ": heading_position, controls_position and status_position must order "
				"the heading, eight rows and status without overlapping the help column.");
		}

		// Validate the shared page layout first so its diagnostics still identify
		// a malformed controls column before the preview placed inside it.
		const JsonValue preview = json.object("stick_preview");
		const auto read_preview_box = [&](const char* key)
		{
			const JsonValue box = preview.object(key);
			return RectangleF(read_finite_vector(box, "position"), read_positive_vector(box, "size"));
		};
		menu.stick_preview.raw_movement = read_preview_box("raw_movement");
		menu.stick_preview.movement_response = read_preview_box("movement_response");
		menu.stick_preview.aim = read_preview_box("aim");
		menu.stick_preview.aim_readout_position = read_finite_vector(preview, "aim_readout_position");
		menu.stick_preview.association_position = read_finite_vector(preview, "association_position");
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
		menu.window_open_sound = json.string("window_open_sound");
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
		if (menu.heading_y >= menu.winner_y || menu.winner_y >= menu.session_score_y ||
			menu.session_score_y >= menu.session_result_y)
		{
			throw std::runtime_error(json.where() +
				": heading_y < winner_y < session_score_y < session_result_y required.");
		}
		menu.contribution_position = read_vector(json, "contribution_position");
		menu.contribution_spacing = read_positive(json, "contribution_spacing");
		if (menu.contribution_spacing > 100.0f)
		{
			throw std::runtime_error(json.where() + ": contribution_spacing must be at most 100.");
		}
		menu.contribution_font = json.string("contribution_font");

		const JsonValue fill = json.object("fill");
		menu.fill_box_size = read_positive_vector(fill, "box_size");
		menu.fill_box_y_offset = fill.number("box_y_offset");
		menu.team_fill_border = read_positive_vector(fill, "border");
		menu.track_colour = read_colour(fill, "track_colour");
		if (menu.track_colour.a != 1.0f || menu.track_colour == menu.art.ink)
		{
			throw std::runtime_error(fill.where() + ": track_colour must be opaque and distinct from art.ink.");
		}
		if (!std::isfinite(menu.fill_box_y_offset) || menu.fill_box_y_offset < 0.0f ||
			menu.fill_box_y_offset > 900.0f || menu.fill_box_size.x > menu.box_size.x ||
			menu.fill_box_size.y > menu.box_size.y || menu.team_fill_border.x < menu.art.keyline_width ||
			menu.team_fill_border.y < menu.art.keyline_width ||
			menu.team_fill_border.x * 2.0f >= menu.fill_box_size.x ||
			menu.team_fill_border.y * 2.0f >= menu.fill_box_size.y)
		{
			throw std::runtime_error(fill.where() +
				": box_size must fit the panel, box_y_offset must be 0..900, and border must contain the keyline and leave positive fill space.");
		}
		menu.team_fill_delay = fill.number("delay");
		// Zero has no defined fill ratio. Reject it here instead of sending a
		// first-frame NaN to the renderer.
		menu.team_fill_time = read_positive(fill, "time");

		// Derive the fill size from the authored box and border.
		menu.team_fill_size =
			menu.fill_box_size - menu.team_fill_border * 2.0f;

		menu.show_results_delay = json.number("show_results_delay");
		if (!std::isfinite(menu.team_fill_delay) || menu.team_fill_delay < 0.0f ||
			menu.team_fill_delay > 10.0f || menu.team_fill_time > 10.0f ||
			!std::isfinite(menu.show_results_delay) || menu.show_results_delay < 0.0f ||
			menu.show_results_delay > 10.0f)
		{
			throw std::runtime_error(json.where() +
				": fill.delay and show_results_delay must be 0..10, and fill.time must be at most 10.");
		}

		const JsonValue team_mark = json.object("team_mark");
		menu.team_mark_size = read_positive_vector(team_mark, "size");
		menu.team_mark_gap = team_mark.number("gap");
		if (!std::isfinite(menu.team_mark_gap) || menu.team_mark_gap < 0.0f || menu.team_mark_gap > 100.0f)
		{
			throw std::runtime_error(team_mark.where() + ": gap must be 0..100.");
		}

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
		const JsonValue paper = json.object("paper");
		hud.paper.ink = read_colour(paper, "ink");
		hud.paper.paper = read_colour(paper, "paper");
		hud.paper.shade = read_colour(paper, "shade");
		hud.paper.keyline_width = read_positive(paper, "keyline_width");
		hud.paper.shadow_offset = paper.number("shadow_offset");
		if (hud.paper.keyline_width > 4.0f ||
			!std::isfinite(hud.paper.shadow_offset) || hud.paper.shadow_offset < 0.0f ||
			hud.paper.shadow_offset > 12.0f || hud.paper.ink.a != 1.0f ||
			hud.paper.paper.a != 1.0f || hud.paper.ink == hud.paper.paper ||
			hud.paper.shade.a <= 0.0f)
		{
			throw std::runtime_error(paper.where() +
				": keyline_width <= 4, shadow_offset from 0 to 12, distinct opaque ink/paper "
				"and visible shade required.");
		}
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

		const JsonValue closing = json.object("closing_clock");
		ClosingClockPresentation& clock = hud.closing_clock;
		clock.ink = read_colour(closing, "ink");
		clock.paper = read_colour(closing, "paper");
		clock.shade = read_colour(closing, "shade");
		clock.text = read_colour(closing, "text");
		clock.accent = read_colour(closing, "accent");
		clock.keyline_width = read_positive(closing, "keyline_width");
		clock.shadow_offset = closing.number("shadow_offset");
		clock.tick_reach = read_positive(closing, "tick_reach");
		clock.beat_duration = read_positive(closing, "beat_duration");
		if (clock.keyline_width > 8.0f || !std::isfinite(clock.shadow_offset) ||
			clock.shadow_offset < 0.0f || clock.shadow_offset > 16.0f ||
			clock.tick_reach > 24.0f || clock.beat_duration > 0.5f ||
			clock.ink.a != 1.0f || clock.paper.a != 1.0f || clock.text.a != 1.0f ||
			clock.accent.a != 1.0f || clock.shade.a <= 0.0f)
		{
			throw std::runtime_error(closing.where() +
				": keyline_width <= 8, shadow_offset from 0 to 16, tick_reach <= 24, "
				"beat_duration <= 0.5, opaque ink/paper/text/accent and visible shade required.");
		}

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

	PlayerIdentityPresentation read_player_identity(const JsonValue& root)
	{
		const JsonValue json = root.object("player_identity");
		PlayerIdentityPresentation identity;
		identity.enabled = json.boolean("enabled");
		identity.world_size = read_positive_vector(json, "world_size");
		identity.world_offset = read_finite_vector(json, "world_offset");
		identity.emphasis_duration = read_positive(json, "emphasis_duration");
		identity.emphasis_scale = read_positive(json, "emphasis_scale");
		identity.ui_size = read_positive_vector(json, "ui_size");
		identity.ui_gap = read_positive(json, "ui_gap");
		identity.background_colour = read_colour(json, "background_colour");
		identity.ink_colour = read_colour(json, "ink_colour");
		identity.border = read_positive(json, "border");
		if (identity.emphasis_scale < 1.0f || identity.emphasis_scale > 2.0f ||
			identity.border * 2.0f >= std::min(identity.ui_size.x, identity.ui_size.y) ||
			identity.background_colour.a != 1.0f || identity.ink_colour.a != 1.0f ||
			identity.background_colour == identity.ink_colour)
		{
			throw std::runtime_error(json.where() +
				": emphasis_scale must be 1..2, border must leave room for the glyph, "
				"and badge colours must be distinct and opaque.");
		}
		return identity;
	}

	std::array<IdleDripPresentation, 5> read_idle_drips(const JsonValue& root)
	{
		const JsonValue json = root.object("idle_drip");
		IdleDripPresentation drip;
		drip.enabled = json.boolean("enabled");
		drip.sprite_sheet = json.identifier("sprite_sheet");
		drip.fill_frame = json.identifier("fill_frame");
		drip.keyline = read_colour(json, "keyline");
		drip.grow_time = read_positive(json, "grow_time");
		drip.stretch_time = read_positive(json, "stretch_time");
		drip.lifetime = read_positive(json, "lifetime");
		drip.gravity = read_positive(json, "gravity");
		drip.keyline_width = read_positive(json, "keyline_width");
		std::array<IdleDripPresentation, 5> tools;
		for (IdleDripPresentation& tool : tools)
		{
			tool = drip;
			tool.enabled = false;
		}
		const JsonValue profiles = json.object("tools");
		const auto read_tool = [&](WeaponType type, const char* name)
		{
			const JsonValue profile = profiles.object(name);
			IdleDripPresentation& tool = tools.at(static_cast<size_t>(type));
			tool.enabled = drip.enabled;
			tool.interval_min = read_positive(profile, "interval_min");
			tool.interval_max = read_positive(profile, "interval_max");
			tool.size = read_positive(profile, "size");
			tool.stretch = read_positive(profile, "stretch");
			tool.nozzle_inset = read_positive(profile, "nozzle_inset");
			tool.nozzle_underside = read_positive(profile, "nozzle_underside");
			if (tool.interval_max < tool.interval_min ||
				tool.keyline_width * 2.0f >= tool.size)
			{
				throw std::runtime_error(profile.where() +
					": intervals must be ordered and keylines must leave visible paint.");
			}
		};
		read_tool(WeaponType::sprayer, "sprayer");
		read_tool(WeaponType::sniper, "sniper");
		read_tool(WeaponType::bucket, "bucket");
		return tools;
	}

	WeaponFeedbackPresentation read_weapon_feedback(const JsonValue& root)
	{
		const JsonValue json = root.object("weapon_feedback");
		WeaponFeedbackPresentation feedback;
		feedback.enabled = json.boolean("enabled");
		feedback.sprite_sheet = json.identifier("sprite_sheet");
		feedback.fill_frame = json.identifier("fill_frame");
		feedback.keyline = read_colour(json, "keyline");
		feedback.highlight = read_colour(json, "highlight");
		feedback.muzzle_duration = read_positive(json, "muzzle_duration");
		feedback.empty_duration = read_positive(json, "empty_duration");
		feedback.recovered_duration = read_positive(json, "recovered_duration");
		feedback.keyline_width = read_positive(json, "keyline_width");
		feedback.status_size = read_positive(json, "status_size");
		feedback.status_offset = read_positive(json, "status_offset");
		feedback.aim_marker_distance = read_positive(json, "aim_marker_distance");
		feedback.aim_marker_size = read_positive(json, "aim_marker_size");
		feedback.aim_marker_stroke = read_positive(json, "aim_marker_stroke");
		if (feedback.aim_marker_distance > 128.0f || feedback.aim_marker_size > 32.0f ||
			feedback.aim_marker_distance <= feedback.aim_marker_size ||
			feedback.aim_marker_stroke > feedback.aim_marker_size / 4.0f ||
			feedback.keyline_width >= feedback.aim_marker_stroke)
		{
			throw std::runtime_error(json.where() +
				": aim marker must stay near the nozzle with distinct, visible strokes "
				"(distance <= 128, size <= 32, distance > size, keyline < stroke <= size / 4).");
		}
		if (feedback.keyline_width * 2.0f >= feedback.status_size)
		{
			throw std::runtime_error(json.where() +
				": 'keyline_width' must leave visible fill inside 'status_size'.");
		}
		const JsonValue sizes = json.object("muzzle_sizes");
		const std::array<const char*, 5> names = { "sprayer", "sniper", "roller", "mister", "bucket" };
		for (size_t index = 0; index < names.size(); ++index)
		{
			const Vector2F size = read_positive_vector(sizes, names[index]);
			if (feedback.keyline_width * 2.0f >= std::min(size.x, size.y))
			{
				throw std::runtime_error(sizes.object(names[index]).where() +
					": 'keyline_width' must leave visible fill inside the muzzle size.");
			}
			feedback.muzzle_sizes[index] = size;
		}
		return feedback;
	}

	BlockedShotPresentation read_blocked_shot(const JsonValue& root)
	{
		const JsonValue json = root.object("blocked_shot");
		BlockedShotPresentation contact;
		contact.enabled = json.boolean("enabled");
		contact.sprite_sheet = json.identifier("sprite_sheet");
		contact.fill_frame = json.identifier("fill_frame");
		contact.highlight = read_colour(json, "highlight");
		contact.keyline = read_colour(json, "keyline");
		contact.duration = read_positive(json, "duration");
		contact.repeat_delay = read_positive(json, "repeat_delay");
		contact.width = read_positive(json, "width");
		contact.reach = read_positive(json, "reach");
		contact.bead_size = read_positive(json, "bead_size");
		contact.stroke_width = read_positive(json, "stroke_width");
		contact.keyline_width = read_positive(json, "keyline_width");
		if (contact.duration > 0.4f || contact.repeat_delay < 0.05f ||
			contact.repeat_delay > 1.0f || contact.width > 48.0f || contact.reach > 48.0f ||
			contact.bead_size > contact.width * 0.5f ||
			contact.stroke_width > contact.width * 0.25f ||
			contact.keyline_width * 2.0f >=
				std::min(contact.stroke_width, contact.bead_size * 0.6f))
		{
			throw std::runtime_error(json.where() +
				": blocked contacts need duration <= 0.4 seconds, repeat_delay in [0.05, 1], "
				"width/reach <= 48, bead_size <= width / 2, stroke_width <= width / 4 "
				"and keylines that leave visible strokes and beads.");
		}
		return contact;
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
		const JsonValue quiet = json.object("quiet");
		auto& motion = figure.quiet;
		motion.enabled = quiet.boolean("enabled");
		motion.settle_time = read_positive(quiet, "settle_time");
		motion.breath_period = read_positive(quiet, "breath_period");
		motion.breath_height = read_positive(quiet, "breath_height");
		motion.shift_period = read_positive(quiet, "shift_period");
		motion.shift_distance = read_positive(quiet, "shift_distance");
		motion.blink_min_interval = read_positive(quiet, "blink_min_interval");
		motion.blink_max_interval = read_positive(quiet, "blink_max_interval");
		motion.blink_duration = read_positive(quiet, "blink_duration");
		motion.lean_distance = read_positive(quiet, "lean_distance");
		motion.lean_acceleration = read_positive(quiet, "lean_acceleration");
		motion.lean_settle_time = read_positive(quiet, "lean_settle_time");
		motion.eyelid_height = read_positive(quiet, "eyelid_height");
		motion.face_colour = read_colour(quiet, "face_colour");
		if (motion.blink_max_interval < motion.blink_min_interval ||
			motion.blink_duration >= motion.blink_min_interval ||
			motion.breath_height >= figure.canvas_size.y / 10.0f ||
			motion.lean_distance + motion.shift_distance >= figure.canvas_size.x / 4.0f)
		{
			throw std::runtime_error(quiet.where() +
				": quiet motion must stay modest and blink intervals must be ordered.");
		}
		const JsonValue eyes = quiet.array("eyes");
		if (eyes.size() != motion.eyes.size())
		{
			throw std::runtime_error(eyes.where() + ": needs the two authored eye rectangles.");
		}
		for (size_t index = 0; index < motion.eyes.size(); ++index)
		{
			const JsonValue eye = eyes.at(index);
			const RectangleF rectangle(eye.number("x"), eye.number("y"),
				read_positive(eye, "width"), read_positive(eye, "height"));
			if (!(rectangle.x >= 0.0f && rectangle.y >= 0.0f &&
				rectangle.right() <= figure.canvas_size.x && rectangle.bottom() <= figure.canvas_size.y &&
				motion.eyelid_height < rectangle.height))
			{
				throw std::runtime_error(eye.where() + ": eye must stay in the canvas and leave room for its lid.");
			}
			motion.eyes[index] = rectangle;
		}
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
		const JsonValue art = json.object("monitor_art");
		MonitorArtPresentation& monitor = opening.monitor_art;
		monitor.ink = read_colour(art, "ink");
		monitor.highlight = read_colour(art, "highlight");
		monitor.power_light = read_colour(art, "power_light");
		monitor.pen_width = read_fraction(art, "pen_width");
		monitor.corner_step = read_fraction(art, "corner_step");
		monitor.shadow_width = read_fraction(art, "shadow_width");
		monitor.vent_count = art.integer("vent_count");
		if (monitor.pen_width <= 0.0f || monitor.pen_width > 0.02f ||
			monitor.corner_step < monitor.pen_width || monitor.corner_step > 0.05f ||
			monitor.shadow_width <= 0.0f || monitor.shadow_width > 0.1f ||
			monitor.vent_count < 1 || monitor.vent_count > 12)
		{
			throw std::runtime_error(art.where() +
				": monitor pen, corner and shadow widths must fit the bezel, "
				"and 'vent_count' must be from 1 to 12.");
		}
		const auto read_monitor_rectangle = [&art](const char* key)
		{
			const JsonValue value = art.object(key);
			const RectangleF rectangle(read_fraction(value, "x"), read_fraction(value, "y"),
				read_fraction(value, "width"), read_fraction(value, "height"));
			if (rectangle.width <= 0.0f || rectangle.height <= 0.0f ||
				rectangle.right() > 1.0f || rectangle.bottom() > 1.0f)
			{
				throw std::runtime_error(value.where() + ": monitor artwork must be on the bezel.");
			}
			return rectangle;
		};
		monitor.vent_area = read_monitor_rectangle("vent_area");
		monitor.power_button = read_monitor_rectangle("power_button");
		monitor.power_indicator = read_monitor_rectangle("power_indicator");
		const JsonValue glass = json.object("glass");
		opening.glass = RectangleF(read_fraction(glass, "x"), read_fraction(glass, "y"),
			read_fraction(glass, "width"), read_fraction(glass, "height"));
		if (opening.glass.width <= 0.0f || opening.glass.height <= 0.0f ||
			opening.glass.right() > 1.0f || opening.glass.bottom() > 1.0f)
		{
			throw std::runtime_error(glass.where() + ": the glass must be on the screen.");
		}
		for (const RectangleF& hardware : { monitor.vent_area, monitor.power_button,
			monitor.power_indicator })
		{
			if (hardware.x < opening.glass.right() && opening.glass.x < hardware.right() &&
				hardware.y < opening.glass.bottom() && opening.glass.y < hardware.bottom())
			{
				throw std::runtime_error(art.where() + ": monitor hardware must stay outside the glass.");
			}
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
		const JsonValue person_art = json.object("silhouette_art");
		SilhouetteArtPresentation& person = opening.silhouette_art;
		person.ink = read_colour(person_art, "ink");
		person.rim = read_colour(person_art, "rim");
		person.shade = read_colour(person_art, "shade");
		person.pen_width = read_fraction(person_art, "pen_width");
		person.shade_width = read_fraction(person_art, "shade_width");
		if (person.pen_width <= 0.0f || person.pen_width > 0.025f ||
			person.shade_width < person.pen_width || person.shade_width > 0.08f)
		{
			throw std::runtime_error(person_art.where() +
				": silhouette pen and shade widths must fit its coarse contour.");
		}
		const JsonValue cursor_art = json.object("cursor_art");
		CursorArtPresentation& cursor = opening.cursor_art;
		cursor.ink = read_colour(cursor_art, "ink");
		cursor.fill = read_colour(cursor_art, "fill");
		cursor.shade = read_colour(cursor_art, "shade");
		cursor.shadow = read_colour(cursor_art, "shadow");
		cursor.shadow_offset = read_fraction(cursor_art, "shadow_offset");
		cursor.click_duration = read_positive(cursor_art, "click_duration");
		cursor.click_reach = read_fraction(cursor_art, "click_reach");
		if (cursor.shadow_offset > 0.2f || cursor.click_duration <= 0.0f ||
			cursor.click_duration > 0.5f || cursor.click_reach <= 0.0f || cursor.click_reach > 0.6f)
		{
			throw std::runtime_error(cursor_art.where() +
				": cursor shadow/click must stay local; 'click_duration' must be in (0, 0.5].");
		}
		opening.adult_height = read_fraction(json, "adult_height");
		opening.child_height = read_fraction(json, "child_height");
		opening.cursor_size = read_fraction(json, "cursor_size");
		opening.preview_strength = read_fraction(json, "preview_strength");
		opening.figure_grey = read_colour(json, "figure_grey");
		opening.percent_font = json.identifier("percent_font");
		opening.percent_height = read_fraction(json, "percent_height");
		const JsonValue reveal_art = json.object("reveal_art");
		RevealArtPresentation& reveal = opening.reveal_art;
		reveal.ink = read_colour(reveal_art, "ink");
		reveal.paper = read_colour(reveal_art, "paper");
		reveal.shade = read_colour(reveal_art, "shade");
		reveal.keyline_width = read_positive(reveal_art, "keyline_width");
		reveal.padding = read_fraction(reveal_art, "padding");
		reveal.shadow_offset = read_fraction(reveal_art, "shadow_offset");
		reveal.tick_length = read_fraction(reveal_art, "tick_length");
		reveal.beat_duration = read_positive(reveal_art, "beat_duration");
		if (reveal.keyline_width > 0.06f || reveal.padding < 0.08f || reveal.padding > 0.18f ||
			reveal.shadow_offset > 0.1f || reveal.tick_length > 0.2f || reveal.beat_duration > 0.5f)
		{
			throw std::runtime_error(reveal_art.where() +
				": keyline_width must be in (0, 0.06], padding in [0.08, 0.18], "
				"shadow_offset in [0, 0.1], tick_length in [0, 0.2] and beat_duration in (0, 0.5].");
		}
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

const std::wstring& MenuText::heading(MenuHeading heading) const
{
	return this->headings.at(static_cast<size_t>(heading));
}

const std::wstring& MenuText::prompt(MenuPrompt prompt) const
{
	return this->prompts.at(static_cast<size_t>(prompt));
}

const std::wstring& MenuText::item(MenuItem item) const
{
	return this->items.at(static_cast<size_t>(item));
}

const WeaponChoicePresentation& WeaponSelectArtworkPresentation::choice(WeaponType type) const
{
	const size_t index = static_cast<size_t>(type);
	if (index >= this->choices.size())
	{
		throw std::out_of_range("Unsupported weapon selection type: " +
			std::to_string(static_cast<int>(type)));
	}
	return this->choices[index];
}

Presentation::Presentation(const char* path)
{
	const JsonDocument document = read_json_file(path);
	this->source_path_ = path;
	const JsonValue root = document.root();

	this->team_palette_ = read_team_palette(root);
	this->menu_ = read_menu_style(root);
	this->menu_text_ = read_menu_text(root);
	const JsonValue music_mix = root.object("music_mix");
	const auto music_duration = [&](const char* key)
	{
		const float value = read_positive(music_mix, key);
		if (value > 5.0f)
		{
			throw std::runtime_error(music_mix.where() + ": " + key + " must be at most 5 seconds.");
		}
		return value;
	};
	this->music_mix_.fade_in = music_duration("fade_in");
	this->music_mix_.fade_out = music_duration("fade_out");
	this->music_mix_.duck_time = music_duration("duck_time");
	this->music_mix_.restore_time = music_duration("restore_time");
	this->music_mix_.pause_gain = music_mix.number("pause_gain");
	if (!std::isfinite(this->music_mix_.pause_gain) ||
		this->music_mix_.pause_gain < 0.0f || this->music_mix_.pause_gain > 1.0f)
	{
		throw std::runtime_error(music_mix.where() + ": pause_gain must be 0..1.");
	}
	this->main_menu_ = read_main_menu(root, this->menu_.footer_y);
	this->controls_settings_ = read_controls_settings(root, this->menu_.footer_y);
	this->pause_menu_ = read_pause_menu(root);
	this->end_menu_ = read_end_menu(root);
	this->results_menu_ = read_results_menu(root);
	this->prompt_bar_ = read_prompt_bar(root);
	this->hud_ = read_hud(root);
	this->player_identity_ = read_player_identity(root);
	this->camera_.reference_height = read_positive(root.object("camera"), "reference_height");
	const JsonValue shake = root.object("camera").object("shake");
	ViewShakePresentation& kick = this->camera_.shake;
	kick.hurt_amplitude = read_positive(shake, "hurt_amplitude");
	kick.elimination_amplitude = read_positive(shake, "elimination_amplitude");
	kick.landing_amplitude = read_positive(shake, "landing_amplitude");
	kick.landing_min_speed = read_positive(shake, "landing_min_speed");
	kick.landing_full_speed = read_positive(shake, "landing_full_speed");
	kick.duration = read_positive(shake, "duration");
	kick.cycles = read_positive(shake, "cycles");
	kick.repeat_delay = read_positive(shake, "repeat_delay");
	if (kick.hurt_amplitude > 8.0f || kick.elimination_amplitude > 8.0f ||
		kick.landing_amplitude > 8.0f || kick.duration > 0.5f || kick.cycles > 3.0f ||
		kick.repeat_delay < kick.duration || kick.repeat_delay > 2.0f ||
		kick.landing_full_speed <= kick.landing_min_speed)
	{
		throw std::runtime_error(shake.where() + ": shake needs amplitudes <= 8 world units, "
			"duration <= 0.5 seconds, cycles <= 3, duration <= repeat_delay <= 2 seconds "
			"and landing_full_speed > landing_min_speed.");
	}
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
	feedback.step = read_movement_burst(movement, "step");
	feedback.skid = read_movement_burst(movement, "skid");
	feedback.keyline = read_colour(movement, "keyline");
	feedback.keyline_width = read_positive(movement, "keyline_width");
	if (feedback.minimum_strength > 1.0f ||
		feedback.full_impact_speed < feedback.landing_min_speed ||
		feedback.takeoff.particle_size < feedback.pixel_size ||
		feedback.landing.particle_size < feedback.pixel_size ||
		feedback.step.particle_size < feedback.pixel_size ||
		feedback.skid.particle_size < feedback.pixel_size ||
		feedback.keyline_width * 2.0f >=
			std::min(feedback.step.particle_size, feedback.skid.particle_size))
	{
		throw std::runtime_error(movement.where() +
			": strength must be at most one, full impact speed must cover minimum speed, "
			"particles must be at least one presentation pixel, and contact keylines "
			"must leave a visible particle interior.");
	}
	const JsonValue combat = root.object("combat_feedback");
	CombatFeedbackPresentation& contact = this->combat_feedback_;
	contact.enabled = combat.boolean("enabled");
	contact.sprite_sheet = combat.string("sprite_sheet");
	contact.fill_frame = combat.string("fill_frame");
	contact.highlight = read_colour(combat, "highlight");
	contact.keyline = read_colour(combat, "keyline");
	contact.hurt_duration = read_positive(combat, "hurt_duration");
	contact.hurt_repeat_delay = read_positive(combat, "hurt_repeat_delay");
	contact.hurt_width = read_positive(combat, "hurt_width");
	contact.hurt_reach = read_positive(combat, "hurt_reach");
	contact.elimination_duration = read_positive(combat, "elimination_duration");
	contact.elimination_spread = read_positive(combat, "elimination_spread");
	contact.elimination_lift = read_positive(combat, "elimination_lift");
	contact.particle_size = read_positive(combat, "particle_size");
	contact.keyline_width = read_positive(combat, "keyline_width");
	if (contact.hurt_repeat_delay < contact.hurt_duration ||
		contact.keyline_width * 2.0f >= std::min(contact.hurt_width, contact.particle_size))
	{
		throw std::runtime_error(combat.where() +
			": repeat delay must cover the hurt duration and keylines must leave visible fill.");
	}
	this->weapon_feedback_ = read_weapon_feedback(root);
	this->blocked_shot_ = read_blocked_shot(root);
	this->idle_drips_ = read_idle_drips(root);
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
	this->match_.stage_entry_size = read_positive_vector(match, "stage_entry_size");
	this->match_.stage_entry_top = read_positive(match, "stage_entry_top");
	const JsonValue countdown_art = match.object("countdown_art");
	CountdownArtPresentation& signal = this->match_.countdown_art;
	signal.ink = read_colour(countdown_art, "ink");
	signal.paper = read_colour(countdown_art, "paper");
	signal.shade = read_colour(countdown_art, "shade");
	signal.number = read_colour(countdown_art, "number");
	signal.go = read_colour(countdown_art, "go");
	signal.keyline_width = read_positive(countdown_art, "keyline_width");
	signal.shadow_offset = countdown_art.number("shadow_offset");
	signal.beat_duration = read_positive(countdown_art, "beat_duration");
	signal.pulse = read_fraction(countdown_art, "pulse");
	if (signal.keyline_width > 8.0f || !std::isfinite(signal.shadow_offset) ||
		signal.shadow_offset < 0.0f || signal.shadow_offset > 16.0f ||
		signal.beat_duration > 0.5f || signal.pulse > 0.15f)
	{
		throw std::runtime_error(countdown_art.where() +
			": keyline_width <= 8, shadow_offset <= 16, beat_duration <= 0.5 and pulse <= 0.15 required.");
	}
	const MatchPresentation& intro = this->match_;
	if (intro.stage_entry_size.x <= intro.controls_padding.x * 2.0f + signal.shadow_offset ||
		intro.stage_entry_size.y <= intro.controls_padding.y * 2.0f + signal.shadow_offset ||
		intro.stage_entry_size.x <= signal.keyline_width * 6.0f + signal.shadow_offset ||
		intro.stage_entry_size.y <= signal.keyline_width * 6.0f + signal.shadow_offset ||
		intro.stage_entry_size.x + intro.controls_margin.x * 2.0f > intro.reference_size.x ||
		intro.stage_entry_top + intro.stage_entry_size.y + intro.controls_gap >
			intro.reference_size.y * intro.countdown_center_y - intro.countdown_size.y / 2.0f)
	{
		throw std::runtime_error(match.where() +
			": stage_entry_size and stage_entry_top must fit above the countdown with room inside their padding.");
	}
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

const CombatFeedbackPresentation& Presentation::combat_feedback() const
{
	return this->combat_feedback_;
}

const WeaponFeedbackPresentation& Presentation::weapon_feedback() const
{
	return this->weapon_feedback_;
}

const BlockedShotPresentation& Presentation::blocked_shot() const
{
	return this->blocked_shot_;
}

const IdleDripPresentation& Presentation::idle_drip(WeaponType type) const
{
	return this->idle_drips_.at(static_cast<size_t>(type));
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
	check.frame("menu_style.fill_frame", this->menu_.sprite_sheet, this->menu_.fill_frame);
	const char* roles[] = { "a", "center", "b" };
	for (size_t index = 0; index < std::size(roles); ++index)
	{
		check.frame(std::string("main_menu.team_select.frames.") + roles[index],
			this->main_menu_.team_select_sheet, this->main_menu_.team_select_frames[index]);
		check.frame(std::string("main_menu.team_select.selected_frames.") + roles[index],
			this->main_menu_.team_select_sheet, this->main_menu_.team_select_selected_frames[index]);
	}
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
	check.font("controls_settings.item_font", this->controls_settings_.item_font);
	check.font("results_menu.contribution_font", this->results_menu_.contribution_font);
	check.font("prompt_bar.font", this->prompt_bar_.font);
	check.font("debug_overlay.font", this->debug_overlay_.font);
	check.font("hud.timer.font", this->hud_.timer_font);
	const auto check_menu_line = [&](const std::string& key, const std::wstring& text,
		const std::string& font, const Vector2F& position, float bottom, const Vector2F& shadow)
	{
		const std::string where = this->source_path_ + ": " + key;
		drawable_text::check_font_can_draw(&render, font, text, where);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
		const float width = 1920.0f - position.x - this->menu_.brand_position.x;
		const float height = bottom - position.y;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x < 0.0f || position.y < 0.0f || !std::isfinite(width) || !std::isfinite(height) ||
			!std::isfinite(shadow.x) || !std::isfinite(shadow.y) ||
			position.x + std::min(0.0f, shadow.x) < 0.0f ||
			position.y + std::min(0.0f, shadow.y) < 0.0f ||
			!std::isfinite(size.x) || !std::isfinite(size.y) ||
			size.x + std::abs(shadow.x) > width || size.y + std::abs(shadow.y) > height)
		{
			throw std::runtime_error(where + " must fit its menu design area including its shadow and right margin.");
		}
	};
	check_menu_line("menu_text.brand", this->menu_text_.brand, this->menu_.brand_font,
		this->menu_.brand_position, this->menu_.footer_y, Vector2F::ZERO);
	for (size_t index = 0; index < this->menu_text_.headings.size(); ++index)
	{
		Vector2F position = this->menu_.layout.position;
		switch (static_cast<MenuHeading>(index))
		{
		case MenuHeading::home: position = this->main_menu_.home_position; break;
		case MenuHeading::options: position = this->main_menu_.options_heading_position; break;
		case MenuHeading::credits: position = this->main_menu_.credits_heading_position; break;
		case MenuHeading::controls_audio:
		case MenuHeading::pad_sticks: position = this->controls_settings_.heading_position; break;
		default: break;
		}
		check_menu_line(std::string("menu_text.headings.") + menu_heading_keys[index],
			this->menu_text_.headings[index], this->menu_.heading_font, position,
			this->menu_.footer_y, this->menu_.heading_shadow_offset);
	}
	for (size_t index = 0; index < this->menu_text_.prompts.size(); ++index)
	{
		check_menu_line(std::string("menu_text.prompts.") + menu_prompt_keys[index],
			this->menu_text_.prompts[index], this->prompt_bar_.font, this->prompt_bar_.position,
			1080.0f, this->prompt_bar_.shadow_offset);
	}
	const auto check_menu_item = [&](MenuItem item, const std::string& font, const Vector2F& position,
		const Vector2F& paper_size, float row_spacing, const std::optional<float>& label_width,
		const MenuPaperArtPresentation& art, float content_height)
	{
		const std::string where = this->source_path_ + ": menu_text.items." +
			menu_item_keys[static_cast<size_t>(item)];
		const std::wstring& text = this->menu_text_.item(item);
		drawable_text::check_font_can_draw(&render, font, text, where);
		Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
		if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
		{
			throw std::runtime_error(where + " must have finite positive measured text bounds.");
		}
		const Vector2F padding = art.focus_padding;
		if (label_width.has_value())
		{
			// Match OptionRow's existing width-then-height fit. Long supported
			// labels remain valid; the display column, not a character limit, fits them.
			const float width = *label_width;
			const float height = row_spacing - padding.y * 2.0f;
			if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
			{
				throw std::runtime_error(where + " must leave a positive finite fitted label column and row height.");
			}
			float scale = 1.0f;
			if (size.x > width) { scale = width / size.x; }
			if (size.y * scale > height) { scale = height / size.y; }
			if (!std::isfinite(scale) || scale <= 0.0f)
			{
				throw std::runtime_error(where + " must leave a positive fitted label column and row height.");
			}
			size *= scale;
		}
		const Vector2F top = (Vector2F(1920.0f, content_height) - paper_size) / 2.0f;
		const Vector2F bottom = top + paper_size;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x - padding.x < top.x || position.y - padding.y < top.y ||
			position.x + size.x + padding.x > bottom.x ||
			position.y + size.y + padding.y > bottom.y ||
			(row_spacing > 0.0f && size.y + padding.y * 2.0f > row_spacing))
		{
			throw std::runtime_error(where + " must fit its paper and row including focus padding.");
		}
		return size;
	};
	const MainMenuPresentation& main = this->main_menu_;
	const MenuItem home_items[]{ MenuItem::play, MenuItem::options, MenuItem::credits, MenuItem::exit };
	for (size_t index = 0; index < std::size(home_items); ++index)
	{
		check_menu_item(home_items[index], this->menu_.item_font, main.home_position +
			Vector2F(0.0f, main.home_spacing.y * static_cast<float>(index + 2)),
			main.home_box_size, main.home_spacing.y, std::nullopt, main.art, this->menu_.footer_y);
	}
	const MenuItem options_items[]{ MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette,
		MenuItem::controls_audio, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(options_items); ++index)
	{
		const Vector2F position = main.options_position +
			Vector2F(0.0f, main.options_spacing.y * static_cast<float>(index));
		const std::optional<float> label_width = index < 3 ?
			std::optional<float>(main.options_value_x - position.x - main.art.focus_padding.x) : std::nullopt;
		check_menu_item(options_items[index], this->menu_.item_font, position,
			main.options_box_size, main.options_spacing.y, label_width, main.art, this->menu_.footer_y);
	}
	check_menu_item(MenuItem::back, this->menu_.item_font, main.credits_back_position, main.credits_box_size, 0.0f, std::nullopt,
		main.art, this->menu_.footer_y);
	const PauseMenuPresentation& pause = this->pause_menu_;
	if (!std::isfinite(pause.layout.spacing.y) || pause.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.items.resume requires positive finite pause_menu.layout.spacing.y.");
	}
	const MenuItem pause_items[]{ MenuItem::resume, MenuItem::restart, MenuItem::controls_audio, MenuItem::quit };
	for (size_t index = 0; index < std::size(pause_items); ++index)
	{
		check_menu_item(pause_items[index], this->menu_.item_font, pause.layout.position +
			Vector2F(0.0f, pause.layout.spacing.y * static_cast<float>(index)),
			pause.box_size, pause.layout.spacing.y, std::nullopt, pause.art, 1080.0f);
	}
	check_menu_item(MenuItem::yes, this->menu_.item_font, pause.layout.position, pause.box_size, pause.layout.spacing.y,
		std::nullopt, pause.art, 1080.0f);
	check_menu_item(MenuItem::no, this->menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		pause.box_size, pause.layout.spacing.y, std::nullopt, pause.art, 1080.0f);
	const EndMenuPresentation& end = this->end_menu_;
	if (!std::isfinite(end.layout.spacing.y) || end.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.items.change_teams requires positive finite end_menu.layout.spacing.y.");
	}
	const MenuItem end_items[]{ MenuItem::change_teams, MenuItem::change_weapons, MenuItem::change_level };
	for (size_t index = 0; index < std::size(end_items); ++index)
	{
		check_menu_item(end_items[index], this->menu_.item_font, end.layout.position +
			Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(index + 1)),
			end.box_size, end.layout.spacing.y, std::nullopt, end.art, 1080.0f);
	}
	// Exit moves up while a series is ongoing; both layouts share this text.
	for (const int row : { 1, 4 })
	{
		check_menu_item(MenuItem::exit_to_main_menu, this->menu_.item_font, end.layout.position +
			Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(row)),
			end.box_size, end.layout.spacing.y, std::nullopt, end.art, 1080.0f);
	}
	const ControlsSettingsPresentation& controls = this->controls_settings_;
	const auto check_controls_item = [&](MenuItem item, int row, bool fitted_label)
	{
		const Vector2F position = controls.layout.position +
			Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(row));
		const std::optional<float> label_width = fitted_label ?
			std::optional<float>(controls.layout.spacing.x - controls.art.focus_padding.x) : std::nullopt;
		const Vector2F size = check_menu_item(item, controls.item_font, position,
			controls.box_size, controls.layout.spacing.y, label_width, controls.art, this->menu_.footer_y);
		if (position.x + size.x + controls.art.focus_padding.x > controls.controls_position.x)
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.items." +
				menu_item_keys[static_cast<size_t>(item)] + " must fit before the controls help column including focus padding.");
		}
		if (position.y + size.y + controls.art.focus_padding.y > controls.status_position.y)
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.items." +
				menu_item_keys[static_cast<size_t>(item)] + " must fit above the controls status line including focus padding.");
		}
	};
	const MenuItem controls_items[]{ MenuItem::pad_sticks, MenuItem::music_volume, MenuItem::sfx_volume,
		MenuItem::view_shake, MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(controls_items); ++index)
	{
		check_controls_item(controls_items[index], static_cast<int>(index), index >= 1 && index <= 5);
	}
	const MenuItem sticks_items[]{ MenuItem::pad, MenuItem::move_deadzone, MenuItem::aim_deadzone,
		MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(sticks_items); ++index)
	{
		check_controls_item(sticks_items[index], static_cast<int>(index), index < 3);
	}
	const auto check_setup_item = [&](MenuItem item, const Vector2F& spacing, int row, bool duration_label)
	{
		const std::string key = std::string("menu_text.items.") + menu_item_keys[static_cast<size_t>(item)];
		const std::string where = this->source_path_ + ": " + key;
		const Vector2F shadow = this->menu_.item_shadow_offset;
		if (!std::isfinite(spacing.y) || spacing.y <= 0.0f ||
			(duration_label && (!std::isfinite(spacing.x) || spacing.x <= 0.0f)))
		{
			throw std::runtime_error(where + " requires positive finite setup row/column spacing.");
		}
		const Vector2F position = this->menu_.layout.position + Vector2F(0.0f, spacing.y * static_cast<float>(row));
		const std::wstring& text = this->menu_text_.item(item);
		check_menu_line(key, text, this->menu_.item_font, position, this->menu_.footer_y, shadow);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(this->menu_.item_font), text);
		if (size.y + std::abs(shadow.y) > spacing.y)
		{
			throw std::runtime_error(where + " must fit its setup row including its item shadow.");
		}
		if (duration_label && size.x + std::abs(shadow.x) > spacing.x * 2.0f)
		{
			// The existing, unfitted OptionRow puts its value two columns later.
			throw std::runtime_error(where + " must end before its duration value column including its item shadow.");
		}
	};
	const MenuItem mode_items[]{ MenuItem::single_match, MenuItem::best_of_three, MenuItem::best_of_five,
		MenuItem::match_length, MenuItem::back };
	for (size_t index = 0; index < std::size(mode_items); ++index)
	{
		check_setup_item(mode_items[index], main.mode_select_spacing, static_cast<int>(index) + 2,
			mode_items[index] == MenuItem::match_length);
	}
	const MenuItem player_items[]{ MenuItem::one_player_practice, MenuItem::two_players,
		MenuItem::three_players, MenuItem::four_players, MenuItem::back };
	for (size_t index = 0; index < std::size(player_items); ++index)
	{
		check_setup_item(player_items[index], main.player_count_spacing, static_cast<int>(index) + 2, false);
	}
	// The nonfocusable hint retains the detail font and the item-shadow offset
	// that its existing widget uses; it has no action-row fit or focus padding.
	check_menu_line("menu_text.solo_practice_hint", this->menu_text_.solo_practice_hint, this->menu_.detail_font,
		this->menu_.layout.position + Vector2F(0.0f, main.mode_select_spacing.y * 7.0f),
		this->menu_.footer_y, this->menu_.item_shadow_offset);
	const auto fixed_menu_size = [&](const std::string& key, const std::wstring& text, const std::string& font)
	{
		const std::string where = this->source_path_ + ": " + key;
		drawable_text::check_font_can_draw(&render, font, text, where);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
		if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
		{
			throw std::runtime_error(where + " must have finite positive measured text bounds.");
		}
		return size;
	};
	const auto check_fixed_paper = [&](const std::string& key, RectangleF bounds,
		const Vector2F& paper_size, const Vector2F& padding)
	{
		bounds.inflate(padding);
		const RectangleF paper((Vector2F(1920.0f, this->menu_.footer_y) - paper_size) / 2.0f, paper_size);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !paper.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its menu paper including any focus padding.");
		}
	};
	const auto centered_title = [&](const std::string& key, const std::wstring& text,
		const std::string& font, float y, const Vector2F& padding)
	{
		const Vector2F size = fixed_menu_size(key, text, font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, y, size.x, size.y);
		check_fixed_paper(key, bounds, main.title_box_size, padding);
		return bounds;
	};
	const MenuTitleText& title = this->menu_text_.title;
	const RectangleF wordmark = centered_title("menu_text.title.wordmark", title.wordmark,
		main.title_font, main.title_y, Vector2F::ZERO);
	const RectangleF subtitle = centered_title("menu_text.title.subtitle", title.subtitle,
		this->menu_.detail_font, main.title_subtitle_y, Vector2F::ZERO);
	RectangleF start = centered_title("menu_text.title.start", title.start,
		this->menu_.item_font, main.title_start_y, main.art.focus_padding);
	start.inflate(main.art.focus_padding);
	const RectangleF no_controller = centered_title("menu_text.title.no_controller", title.no_controller,
		this->menu_.item_font, main.title_no_pad_y, Vector2F::ZERO);
	// Build identity remains generated, but it still shares the title's paper.
	const RectangleF build = centered_title("main_menu.build_id_y", build_info::label,
		this->prompt_bar_.font, main.build_id_y, Vector2F::ZERO);
	const auto precedes = [&](const std::string& key, const RectangleF& before, const RectangleF& after)
	{
		if (before.bottom() > after.y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must end before the next menu text or focus card.");
		}
	};
	precedes("menu_text.title.wordmark", wordmark, subtitle);
	precedes("menu_text.title.subtitle", subtitle, start);
	precedes("menu_text.title.start", start, no_controller);
	precedes("menu_text.title.no_controller", no_controller, build);
	const RectangleF home_subtitle(main.home_subtitle_position,
		fixed_menu_size("menu_text.home_subtitle", this->menu_text_.home_subtitle, this->menu_.detail_font));
	check_fixed_paper("menu_text.home_subtitle", home_subtitle, main.home_box_size, Vector2F::ZERO);
	const RectangleF home_heading(main.home_position,
		fixed_menu_size("menu_text.headings.home", this->menu_text_.heading(MenuHeading::home), this->menu_.heading_font));
	check_fixed_paper("menu_text.headings.home", home_heading, main.home_box_size, Vector2F::ZERO);
	precedes("menu_text.home_subtitle", home_heading, home_subtitle);
	RectangleF play(main.home_position + Vector2F(0.0f, main.home_spacing.y * 2.0f),
		fixed_menu_size("menu_text.items.play", this->menu_text_.item(MenuItem::play), this->menu_.item_font));
	play.inflate(main.art.focus_padding);
	precedes("menu_text.home_subtitle", home_subtitle, play);
	check.font("hud.meter_label_font", this->hud_.meter_label_font);
	check.font("hud.player_identity.font", this->hud_.player_identity_font);
	check.frame("hud.fill_frame", this->hud_.sprite_sheet, this->hud_.fill_frame);
	check.frame("movement_feedback.fill_frame", this->movement_feedback_.sprite_sheet,
		this->movement_feedback_.fill_frame);
	check.frame("combat_feedback.fill_frame", this->combat_feedback_.sprite_sheet,
		this->combat_feedback_.fill_frame);
	check.frame("weapon_feedback.fill_frame", this->weapon_feedback_.sprite_sheet,
		this->weapon_feedback_.fill_frame);
	check.frame("blocked_shot.fill_frame", this->blocked_shot_.sprite_sheet,
		this->blocked_shot_.fill_frame);
	check.frame("idle_drip.fill_frame", this->idle_drip().sprite_sheet,
		this->idle_drip().fill_frame);
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
	check.effect("pause_menu.sounds.resume.name", bank, this->pause_menu_.resume_sound.name);
	check.wave("end_menu.sounds.series.name", bank, this->end_menu_.series_sound.name);
	check.wave("results_menu.sounds.winner.name", bank, this->results_menu_.winner_sound.name);
	check.wave("results_menu.sounds.draw.name", bank, this->results_menu_.draw_sound.name);
	check.effect("results_menu.sounds.fill.name", bank, this->results_menu_.fill_sound.name);
	check.effect("results_menu.music.name", bank, this->results_menu_.music.name);

	// Non-menu primitive drawing and developer overlays still use this texel.
	check.frame("built-in primitive frame 'pixel'", "sprite_sheet_1", "pixel");
	const WeaponSelectArtworkPresentation& artwork = this->main_menu_.weapon_select_art;
	check.frame("main_menu.weapon_select.fill_frame", artwork.sheet, artwork.fill_frame);
	const SpriteSheet* weapon_sheet = render.sprite_sheet(artwork.sheet);
	const float name_width = this->main_menu_.weapon_description_x_offset;
	if (!std::isfinite(name_width) || name_width <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ +
			": main_menu.weapon_description.x_offset must be finite and positive for the weapon name column.");
	}
	const FontHandle name_font = render.resolve_sprite_font(this->main_menu_.weapon_description_font);
	for (size_t index = 0; index < artwork.choices.size(); ++index)
	{
		const WeaponChoicePresentation& choice = artwork.choices[index];
		const std::string key = std::string("main_menu.weapon_select.choices.") + weapon_choice_keys[index];
		drawable_text::check_font_can_draw(&render, this->main_menu_.weapon_description_font,
			choice.name, this->source_path_ + ": " + key + ".name");
		const Vector2F name_size = render.measure_text(name_font, choice.name);
		if (!std::isfinite(name_size.x) ||
			name_size.x + std::abs(this->menu_.detail_shadow_offset.x) > name_width)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				".name must fit the name column before main_menu.weapon_description.x_offset.");
		}
		check.frame(key + ".frame", artwork.sheet, choice.frame);
		const SpriteFrame& body = weapon_sheet->sprite_frame(weapon_sheet->resolve_sprite_frame(choice.frame));
		if (body.source_rectangle().width <= 0 || body.source_rectangle().height <= 0)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				".frame must have positive source dimensions.");
		}
		if (!choice.paint_frame.empty())
		{
			check.frame(key + ".paint_frame", artwork.sheet, choice.paint_frame);
			const SpriteFrame& paint = weapon_sheet->sprite_frame(
				weapon_sheet->resolve_sprite_frame(choice.paint_frame));
			if (body.source_rectangle().size() != paint.source_rectangle().size() || body.origin() != paint.origin())
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					".paint_frame must match its body dimensions and origin.");
			}
		}
		if (!choice.supply_strip.empty())
		{
			check.animation(key + ".supply_strip", artwork.sheet, choice.supply_strip);
			const AnimationStrip& supply = weapon_sheet->animation_strip(
				weapon_sheet->resolve_animation_strip(choice.supply_strip));
			if (supply.frame_count() <= 0 || body.origin() != Vector2F::ZERO)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					".supply_strip needs at least one frame and a body with zero authored origin.");
			}
			for (int frame = 0; frame < supply.frame_count(); ++frame)
			{
				if (supply.frame_rect(frame).size() != body.source_rectangle().size())
				{
					throw std::runtime_error(this->source_path_ + ": " + key +
						".supply_strip must retain its body dimensions for every frame.");
				}
			}
		}
	}
}

const MenuStyle& Presentation::menu() const
{
	return this->menu_;
}

const MenuText& Presentation::menu_text() const
{
	return this->menu_text_;
}

const MusicMixPresentation& Presentation::music_mix() const
{
	return this->music_mix_;
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

const PlayerIdentityPresentation& Presentation::player_identity() const
{
	return this->player_identity_;
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
