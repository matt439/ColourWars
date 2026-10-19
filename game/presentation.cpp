#include "game/pch.h"
#include "game/presentation.h"
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

using namespace labrador;
using namespace mattmath;
using namespace content_value;

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
		"reduce_effects", "pad", "move_deadzone", "aim_deadzone", "restore_defaults", "keep", "revert"
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

	std::wstring read_menu_block(const JsonValue& json, const char* key)
	{
		std::wstring text = drawable_text::widen_drawable(json.string(key), json.where() + ": " + key);
		if (text.find(L'\r') != std::wstring::npos)
		{
			throw std::runtime_error(json.where() + ": '" + key + "' must use LF for authored line breaks.");
		}
		size_t begin = 0;
		do
		{
			const size_t end = text.find(L'\n', begin);
			if (text.substr(begin, end == std::wstring::npos ? end : end - begin).find_first_not_of(L' ') ==
				std::wstring::npos)
			{
				throw std::runtime_error(json.where() + ": '" + key + "' must have a nonblank line between each authored LF break.");
			}
			if (end == std::wstring::npos) { break; }
			begin = end + 1;
		} while (begin <= text.size());
		return text;
	}

	struct LabelToken
	{
		std::wstring_view name;
		std::wstring value;
	};

	std::wstring format_label_template(const std::wstring& text,
		std::initializer_list<LabelToken> tokens)
	{
		if (text.find_first_not_of(L' ') == std::wstring::npos)
		{
			throw std::invalid_argument("Menu label text must be nonblank and single-line.");
		}
		for (const wchar_t character : text)
		{
			if (character < L' ' || character > L'~')
			{
				throw std::invalid_argument("Menu label text must use drawable single-line characters.");
			}
		}
		std::wstring result;
		std::vector<std::wstring_view> used;
		for (size_t at = 0; at < text.size();)
		{
			if (text[at] != L'{')
			{
				if (text[at] == L'}')
				{
					throw std::invalid_argument("Menu label text contains an unknown brace.");
				}
				result += text[at++];
				continue;
			}
			const size_t end = text.find(L'}', at);
			const std::wstring_view name = std::wstring_view(text).substr(at,
				end == std::wstring::npos ? end : end - at + 1);
			const auto token = std::find_if(tokens.begin(), tokens.end(),
				[&name](const LabelToken& candidate) { return candidate.name == name; });
			if (token == tokens.end() || std::find(used.begin(), used.end(), name) != used.end())
			{
				throw std::invalid_argument("Menu label text requires each declared token exactly once and no other braces.");
			}
			used.push_back(name);
			// Scan only the original template. Inserted words cannot become
			// another token, and nested fragments cannot manufacture one.
			result += token->value;
			at = end + 1;
		}
		if (used.size() != tokens.size())
		{
			throw std::invalid_argument("Menu label text requires each declared token exactly once.");
		}
		return result;
	}

	struct LabelPen
	{
		float x = 0.0f;
		float y = 0.0f;
		Vector2F extent = Vector2F::ZERO;
	};

	void append_label_glyph(LabelPen& pen, const Glyph& glyph, wchar_t character,
		float line_spacing, const std::string& where)
	{
		if (!std::isfinite(glyph.x_offset) || !std::isfinite(glyph.y_offset) ||
			!std::isfinite(glyph.x_advance))
		{
			throw std::runtime_error(where + " requires finite font glyph metrics.");
		}
		// Keep Font::for_each_glyph/measure's FLOAT operation order, including
		// carried bearings, the zero clamp, whitespace suppression and overhang.
		pen.x += glyph.x_offset;
		if (pen.x < 0.0f) { pen.x = 0.0f; }
		const float width = static_cast<float>(glyph.subrect.width);
		if (character != L' ' || glyph.subrect.width > 1 || glyph.subrect.height > 1)
		{
			const float height = character == L' ' ? line_spacing :
				std::max(line_spacing, static_cast<float>(glyph.subrect.height) + glyph.y_offset);
			pen.extent.x = std::max(pen.extent.x, pen.x + width);
			pen.extent.y = std::max(pen.extent.y, pen.y + height);
		}
		pen.x += width + glyph.x_advance;
		if (!std::isfinite(pen.x) || !std::isfinite(pen.y) ||
			!std::isfinite(pen.extent.x) || !std::isfinite(pen.extent.y))
		{
			throw std::runtime_error(where + " requires finite composed text metrics.");
		}
	}

	void append_label_text(LabelPen& pen, const Font& font, std::wstring_view text,
		const std::string& where)
	{
		if (!std::isfinite(font.line_spacing()) || font.line_spacing() <= 0.0f)
		{
			throw std::runtime_error(where + " requires finite positive font line spacing.");
		}
		for (const wchar_t character : text)
		{
			if (character == L'\r') { continue; }
			if (character == L'\n')
			{
				pen.x = 0.0f;
				pen.y += font.line_spacing();
				continue;
			}
			append_label_glyph(pen, font.drawn(static_cast<char32_t>(character)),
				character, font.line_spacing(), where);
		}
	}

	struct StickReadoutToken
	{
		std::wstring_view name;
		std::wstring value;
	};

	void validate_stick_template(const std::wstring& text,
		std::initializer_list<StickReadoutToken> tokens, bool multiline)
	{
		for (const wchar_t character : text)
		{
			if ((character < L' ' || character > L'~') && !(multiline && character == L'\n'))
			{
				throw std::invalid_argument("Stick readout text must use drawable characters and its allowed LF line breaks.");
			}
		}
		size_t begin = 0;
		do
		{
			const size_t end = text.find(L'\n', begin);
			if (text.substr(begin, end == std::wstring::npos ? end : end - begin).find_first_not_of(L' ') ==
				std::wstring::npos)
			{
				throw std::invalid_argument("Stick readout text must have a nonblank line between each authored LF break.");
			}
			if (end == std::wstring::npos) { break; }
			begin = end + 1;
		} while (begin <= text.size());
		// Count in the original template: erasing one token must never create
		// a different required token from nested fragments around it.
		for (const StickReadoutToken& token : tokens)
		{
			const size_t at = text.find(token.name);
			if (at == std::wstring::npos || text.find(token.name, at + token.name.size()) != std::wstring::npos)
			{
				throw std::invalid_argument("Stick readout text requires each declared token exactly once.");
			}
		}
		begin = 0;
		for (;;)
		{
			const size_t at = text.find_first_of(L"{}", begin);
			if (at == std::wstring::npos) { break; }
			const size_t end = text.find(L'}', at);
			bool known = false;
			if (text[at] == L'{' && end != std::wstring::npos)
			{
				const std::wstring_view name = std::wstring_view(text).substr(at, end - at + 1);
				for (const StickReadoutToken& token : tokens)
				{
					known = known || name == token.name;
				}
			}
			if (!known)
			{
				throw std::invalid_argument("Stick readout text requires each declared token exactly once and no other braces.");
			}
			begin = end + 1;
		}
	}

	std::wstring format_stick_template(const std::wstring& text,
		std::initializer_list<StickReadoutToken> tokens, bool multiline)
	{
		validate_stick_template(text, tokens, multiline);
		std::wstring result = text;
		for (const StickReadoutToken& token : tokens)
		{
			result.replace(result.find(token.name), token.name.size(), token.value);
		}
		return result;
	}

	std::wstring stick_percent_value(int value)
	{
		if (value < -100 || value > 100)
		{
			throw std::invalid_argument("Stick readout percentages must be between -100 and 100.");
		}
		return std::to_wstring(value);
	}

	MenuText read_menu_text(const JsonValue& root)
	{
		const JsonValue json = root.object("menu_text");
		MenuText text;
		text.brand = read_menu_line(json, "brand");
		text.solo_practice_hint = read_menu_line(json, "solo_practice_hint");
		text.options_palette_timing = read_menu_line(json, "options_palette_timing");
		text.display_trial_countdown = read_menu_line(json, "display_trial_countdown");
		const std::wstring countdown_token = L"{seconds}";
		const size_t countdown_at = text.display_trial_countdown.find(countdown_token);
		std::wstring countdown_literals = text.display_trial_countdown;
		if (countdown_at != std::wstring::npos) { countdown_literals.erase(countdown_at, countdown_token.size()); }
		if (countdown_at == std::wstring::npos || countdown_literals.find_first_of(L"{}") != std::wstring::npos)
		{
			throw std::runtime_error(json.where() +
				": display_trial_countdown must contain exactly one {seconds} token and no other braces.");
		}
		const JsonValue title = json.object("title");
		text.title.wordmark = read_menu_block(title, "wordmark");
		text.title.subtitle = read_menu_line(title, "subtitle");
		text.title.start = read_menu_line(title, "start");
		text.title.no_controller = read_menu_line(title, "no_controller");
		text.home_subtitle = read_menu_line(json, "home_subtitle");
		const JsonValue help = json.object("help");
		text.help.controls = read_menu_block(help, "controls");
		text.help.pad_association = read_menu_block(help, "pad_association");
		text.help.aim_cutoff = read_menu_line(help, "aim_cutoff");
		const JsonValue status = json.object("status");
		text.status.options_pending = read_menu_line(status, "options_pending");
		text.status.options_saved = read_menu_line(status, "options_saved");
		text.status.options_save_failed = read_menu_line(status, "options_save_failed");
		text.status.controls_pending = read_menu_line(status, "controls_pending");
		text.status.controls_saved = read_menu_line(status, "controls_saved");
		text.status.active_save_failed = read_menu_line(status, "active_save_failed");
		text.status.sticks_pending = read_menu_line(status, "sticks_pending");
		text.status.sticks_saved = read_menu_line(status, "sticks_saved");
		text.status.options_reverted = read_menu_line(status, "options_reverted");
		text.status.options_display_failed = read_menu_line(status, "options_display_failed");
		text.status.options_rollback_failed = read_menu_line(status, "options_rollback_failed");
		const JsonValue values = json.object("values");
		text.values.on = read_menu_line(values, "on");
		text.values.off = read_menu_line(values, "off");
		text.values.random = read_menu_line(values, "random");
		text.values.percentage = read_menu_line(values, "percentage");
		try
		{
			std::ignore = format_label_template(text.values.percentage, { { L"{value}", L"0" } });
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(values.where() + ": percentage: " + error.what());
		}
		for (const auto& field : { std::pair("duration_minutes", &text.values.duration_minutes),
			std::pair("duration_default", &text.values.duration_default) })
		{
			*field.second = read_menu_line(values, field.first);
			try
			{
				std::ignore = format_label_template(*field.second, { { L"{value}", L"0" } });
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(values.where() + ": " + field.first + ": " + error.what());
			}
		}
		text.values.resolution = read_menu_line(values, "resolution");
		try
		{
			std::ignore = format_label_template(text.values.resolution,
				{ { L"{width}", L"1" }, { L"{height}", L"1" } });
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(values.where() + ": resolution: " + error.what());
		}
		const JsonValue confirmations = json.object("confirmations");
		text.confirmations.restart = read_menu_line(confirmations, "restart");
		text.confirmations.quit = read_menu_line(confirmations, "quit");
		text.confirmations.exit = read_menu_line(confirmations, "exit");
		text.confirmations.display = read_menu_line(confirmations, "display");
		const JsonValue end = json.object("end");
		text.end.match_complete = read_menu_line(end, "match_complete");
		text.end.round_complete = read_menu_line(end, "round_complete");
		text.end.series_complete = read_menu_line(end, "series_complete");
		text.end.rematch = read_menu_line(end, "rematch");
		text.end.play_series_again = read_menu_line(end, "play_series_again");
		text.end.replay_draw = read_menu_line(end, "replay_draw");
		text.end.next_round = read_menu_line(end, "next_round");
		const JsonValue results = json.object("results");
		text.results.single_match_heading = read_menu_line(results, "single_match_heading");
		text.results.proceed = read_menu_line(results, "proceed");
		text.results.contribution_heading = read_menu_line(results, "contribution_heading");
		const JsonValue stage = json.object("stage");
		text.stage.browse_status = read_menu_line(stage, "browse_status");
		text.stage.locked_status = read_menu_line(stage, "locked_status");
		text.stage.briefing_label = read_menu_line(stage, "briefing_label");
		text.stage.goal = read_menu_line(stage, "goal");
		text.stage.solo_objective = read_menu_block(stage, "solo_objective");
		text.stage.versus_objective = read_menu_block(stage, "versus_objective");
		text.stage.random_navigation = read_menu_line(stage, "random_navigation");
		text.stage.locked_hint = read_menu_block(stage, "locked_hint");
		text.stage.browse_hint = read_menu_block(stage, "browse_hint");
		text.stage.solo_mode = read_menu_line(stage, "solo_mode");
		text.stage.single_match_mode = read_menu_line(stage, "single_match_mode");
		const auto read_stage_summary = [&](const char* key, const wchar_t* token)
		{
			const std::wstring value = read_menu_line(stage, key);
			try
			{
				std::ignore = format_label_template(value, { { token, L"1" } });
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(stage.where() + ": " + key + ": " + error.what());
			}
			return value;
		};
		text.stage.best_of_mode = read_stage_summary("best_of_mode", L"{best_of}");
		text.stage.solo_roster = read_stage_summary("solo_roster", L"{players}");
		text.stage.versus_roster = read_stage_summary("versus_roster", L"{players}");
		text.stage.numbered_navigation = read_menu_line(stage, "numbered_navigation");
		try
		{
			std::ignore = format_label_template(text.stage.numbered_navigation,
				{ { L"{index}", L"1" }, { L"{total}", L"1" } });
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(stage.where() + ": numbered_navigation: " + error.what());
		}
		text.stage.random_hint = read_menu_block(stage, "random_hint");
		try
		{
			std::ignore = format_stick_template(text.stage.random_hint, { { L"{count}", L"1" } }, true);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(stage.where() + ": random_hint: " + error.what());
		}
		const JsonValue records = json.object("records");
		text.records.new_best = read_menu_line(records, "new_best");
		text.records.stage_best = read_menu_line(records, "stage_best");
		text.records.best_win = read_menu_line(records, "best_win");
		text.records.best_coverage = read_menu_line(records, "best_coverage");
		text.records.played = read_menu_line(records, "played");
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
		const JsonValue labels = json.object("labels");
		const auto read_label = [&](const char* key, std::initializer_list<LabelToken> tokens)
		{
			const std::wstring value = read_menu_line(labels, key);
			try
			{
				std::ignore = format_label_template(value, tokens);
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(labels.where() + ": " + key + ": " + error.what());
			}
			return value;
		};
		text.labels.pad = read_label("pad", { { L"{pad}", L"1" } });
		text.labels.keyboard_mouse = read_label("keyboard_mouse", {});
		text.labels.waiting = read_label("waiting", {});
		text.labels.roster_row = read_label("roster_row",
			{ { L"{player}", L"P1" }, { L"{device}", L"Pad 1" } });
		const JsonValue connection = json.object("connection");
		text.connection.join_reconnect_suffix = read_menu_line(connection, "join_reconnect_suffix");
		text.connection.join_ready = read_menu_line(connection, "join_ready");
		text.connection.join_waiting = read_menu_line(connection, "join_waiting");
		text.connection.pause_title = read_menu_line(connection, "pause_title");
		text.connection.pause_player_title = read_menu_line(connection, "pause_player_title");
		text.connection.controller_lost = read_menu_line(connection, "controller_lost");
		text.connection.all_connected = read_menu_line(connection, "all_connected");
		text.connection.teams_required = read_menu_line(connection, "teams_required");
		const std::wstring player_token = L"{player}";
		const size_t player_at = text.connection.pause_player_title.find(player_token);
		std::wstring player_literals = text.connection.pause_player_title;
		if (player_at != std::wstring::npos) { player_literals.erase(player_at, player_token.size()); }
		if (player_at == std::wstring::npos || player_literals.find_first_of(L"{}") != std::wstring::npos)
		{
			throw std::runtime_error(connection.where() +
				": pause_player_title must contain exactly one {player} token and no other braces.");
		}
		const JsonValue stick_readouts = json.object("stick_readouts");
		const auto read_stick = [&](const char* key, bool multiline,
			std::initializer_list<StickReadoutToken> tokens)
		{
			const std::wstring value = multiline ? read_menu_block(stick_readouts, key) : read_menu_line(stick_readouts, key);
			try
			{
				validate_stick_template(value, tokens, multiline);
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(stick_readouts.where() + ": " + key + ": " + error.what());
			}
			return value;
		};
		text.stick_readouts.connected = read_stick("connected", false, { { L"{device}", L"" } });
		text.stick_readouts.disconnected = read_stick("disconnected", false, { { L"{device}", L"" } });
		text.stick_readouts.raw_movement = read_stick("raw_movement", false, { { L"{value}", L"" } });
		text.stick_readouts.movement_response = read_stick("movement_response", false, { { L"{value}", L"" } });
		text.stick_readouts.aim_inactive = read_stick("aim_inactive", true,
			{ { L"{raw_x}", L"" }, { L"{raw_y}", L"" } });
		text.stick_readouts.aim_active = read_stick("aim_active", true,
			{ { L"{raw_x}", L"" }, { L"{raw_y}", L"" }, { L"{aim_x}", L"" }, { L"{aim_y}", L"" } });
		return text;
	}

	MovementBurstPresentation read_movement_burst(const JsonValue& parent,
		const char* key)
	{
		const JsonValue json = parent.object(key);
		return { read_positive(json, "duration"), read_positive(json, "spread"),
			read_positive(json, "lift"), read_positive(json, "particle_size") };
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
		// Margins and the meter box are fractions of the player's view.
		hud.right_margin = read_unit(json, "right_margin");
		hud.ammo_top_margin = read_unit(json, "ammo_top_margin");
		hud.health_top_margin = read_unit(json, "health_top_margin");
		hud.box_size = read_positive_vector(json, "box_size");
		hud.box_fill_border = read_non_negative(json, "box_fill_border");
		hud.health_colour = read_colour(json, "health_colour");

		hud.sprite_sheet = json.string("sprite_sheet");
		hud.fill_frame = json.string("fill_frame");

		const JsonValue timer = json.object("timer");
		hud.timer_font = timer.string("font");
		hud.timer_offset = read_vector_unchecked(timer, "offset");
		hud.timer_shadow_offset = read_vector_unchecked(timer, "shadow_offset");
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
		hud.player_identity_offset = read_vector_unchecked(identity, "offset");
		hud.player_identity_shadow_offset = read_vector_unchecked(identity, "shadow_offset");
		hud.player_identity_colour = read_colour(identity, "colour");
		hud.player_identity_shadow_colour = read_colour(identity, "shadow_colour");
		hud.player_identity_background_colour = read_colour(identity, "background_colour");
		hud.player_identity_padding = read_vector_unchecked(identity, "padding");
		hud.player_identity_reference_width = read_positive(identity, "reference_width");


		return hud;
	}

	PlayerIdentityPresentation read_player_identity(const JsonValue& root)
	{
		const JsonValue json = root.object("player_identity");
		PlayerIdentityPresentation identity;
		identity.enabled = json.boolean("enabled");
		identity.world_size = read_positive_vector(json, "world_size");
		identity.world_offset = read_vector(json, "world_offset");
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
		figure.rest_arms_frame = json.identifier("rest_arms_frame");
		figure.rest_arms_paint_frame = json.identifier("rest_arms_paint_frame");
		figure.sleeves_frame = json.identifier("sleeves_frame");
		figure.sleeves_paint_frame = json.identifier("sleeves_paint_frame");
		figure.canvas_size = read_positive_vector(json, "canvas_size");
		figure.body_inset = read_non_negative(json, "body_inset");
		if (!(figure.body_width() > 0.0f))
		{
			throw std::runtime_error(json.where() + ": 'body_inset' must leave the hitbox inside 'canvas_size'.");
		}
		figure.rear_shoulder = read_vector(json, "rear_shoulder");
		figure.front_shoulder = read_vector(json, "front_shoulder");
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
			motion.lean_distance + motion.shift_distance >= figure.body_width() / 4.0f)
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
			figure.grips[index] = { read_vector(grip, "rear"), read_vector(grip, "front") };
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
		// Fractions of the respawn wait.
		redraw.draw_from = read_unit(json, "draw_from");
		redraw.fill_at = read_unit(json, "fill_at");
		if (!(redraw.draw_from < redraw.fill_at))
		{
			throw std::runtime_error(json.where() + ": 'draw_from' must come before 'fill_at'.");
		}
		redraw.camera_hold = read_non_negative(json, "camera_hold");

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
		monitor.pen_width = read_unit(art, "pen_width");
		monitor.corner_step = read_unit(art, "corner_step");
		monitor.shadow_width = read_unit(art, "shadow_width");
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
			const RectangleF rectangle(read_unit(value, "x"), read_unit(value, "y"),
				read_unit(value, "width"), read_unit(value, "height"));
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
		opening.glass = RectangleF(read_unit(glass, "x"), read_unit(glass, "y"),
			read_unit(glass, "width"), read_unit(glass, "height"));
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
		opening.palette_height = read_unit(json, "palette_height");
		opening.palette_sheet = json.identifier("palette_sheet");
		const JsonValue palette = json.array("palette_frames");
		if (opening.palette_sheet.empty() || palette.size() == 0)
		{
			throw std::runtime_error(json.where() +
				": 'palette_sheet' and 'palette_frames' must not be empty.");
		}
		for (size_t index = 0; index < palette.size(); ++index)
		{
			const JsonValue entry = palette.at(index);
			const std::string frame = entry.as_identifier();
			if (frame.empty())
			{
				throw std::runtime_error(entry.where() + ": palette frame must not be empty.");
			}
			opening.palette_frames.push_back(frame);
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
		person.pen_width = read_unit(person_art, "pen_width");
		person.shade_width = read_unit(person_art, "shade_width");
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
		cursor.shadow_offset = read_unit(cursor_art, "shadow_offset");
		cursor.click_duration = read_positive(cursor_art, "click_duration");
		cursor.click_reach = read_unit(cursor_art, "click_reach");
		if (cursor.shadow_offset > 0.2f || cursor.click_duration <= 0.0f ||
			cursor.click_duration > 0.5f || cursor.click_reach <= 0.0f || cursor.click_reach > 0.6f)
		{
			throw std::runtime_error(cursor_art.where() +
				": cursor shadow/click must stay local; 'click_duration' must be in (0, 0.5].");
		}
		opening.adult_height = read_unit(json, "adult_height");
		opening.child_height = read_unit(json, "child_height");
		opening.cursor_size = read_unit(json, "cursor_size");
		opening.preview_strength = read_unit(json, "preview_strength");
		opening.figure_grey = read_colour(json, "figure_grey");
		opening.percent_font = json.identifier("percent_font");
		opening.percent_height = read_unit(json, "percent_height");
		const JsonValue reveal_art = json.object("reveal_art");
		RevealArtPresentation& reveal = opening.reveal_art;
		reveal.ink = read_colour(reveal_art, "ink");
		reveal.paper = read_colour(reveal_art, "paper");
		reveal.shade = read_colour(reveal_art, "shade");
		reveal.keyline_width = read_positive(reveal_art, "keyline_width");
		reveal.padding = read_unit(reveal_art, "padding");
		reveal.shadow_offset = read_unit(reveal_art, "shadow_offset");
		reveal.tick_length = read_unit(reveal_art, "tick_length");
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
		opening.hop = read_unit(acting, "hop");
		opening.shake = read_unit(acting, "shake");
		opening.slump = read_unit(acting, "slump");
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
		debug.font_scale = read_positive(json, "font_scale");
		debug.line_spacing = read_positive(json, "line_spacing");
		debug.position = read_vector_unchecked(json, "position");
		debug.colour = read_colour(json, "colour");
		debug.shadow_colour = read_colour(json, "shadow_colour");
		debug.shadow_offset = read_vector_unchecked(json, "shadow_offset");
		debug.shadow_scale = read_positive(json, "shadow_scale");
		const JsonValue station = json.object("station_label");
		debug.station_label.font = station.string("font");
		debug.station_label.scale = read_positive(station, "scale");
		debug.station_label.offset = read_vector_unchecked(station, "offset");
		debug.station_label.colour = read_colour(station, "colour");
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

std::wstring MenuText::display_countdown(int seconds) const
{
	if (seconds < 0) { throw std::out_of_range("Display countdown seconds must be nonnegative."); }
	std::wstring result = this->display_trial_countdown;
	const std::wstring token = L"{seconds}";
	const size_t at = result.find(token);
	if (at == std::wstring::npos) { throw std::invalid_argument("display_trial_countdown requires {seconds}."); }
	result.replace(at, token.size(), std::to_wstring(seconds));
	return result;
}

std::wstring MenuText::pause_title_for_player(int number) const
{
	if (number < 1 || number > 4)
	{
		throw std::invalid_argument("Pause title requires a player number from 1 to 4.");
	}
	std::wstring result = this->connection.pause_player_title;
	const std::wstring token = L"{player}";
	const size_t at = result.find(token);
	if (at == std::wstring::npos)
	{
		throw std::invalid_argument("pause_player_title requires exactly one {player} token.");
	}
	result.replace(at, token.size(), std::to_wstring(number));
	if (result.find_first_of(L"{}") != std::wstring::npos)
	{
		throw std::invalid_argument("pause_player_title requires exactly one {player} token and no other braces.");
	}
	return result;
}

std::wstring MenuText::device_label(int slot) const
{
	if (input_slots::is_pad(slot))
	{
		return format_label_template(this->labels.pad, { { L"{pad}", std::to_wstring(slot + 1) } });
	}
	if (input_slots::is_keyboard_and_mouse(slot))
	{
		return format_label_template(this->labels.keyboard_mouse, {});
	}
	if (slot == -1)
	{
		return format_label_template(this->labels.waiting, {});
	}
	throw std::invalid_argument("Menu device label requires a slot from -1 to 4.");
}

std::wstring MenuText::setting_percentage(int value) const
{
	if (value < 0 || value > 100)
	{
		throw std::invalid_argument("Setting percentage requires an integer from 0 to 100.");
	}
	return format_label_template(this->values.percentage, { { L"{value}", std::to_wstring(value) } });
}

std::wstring MenuText::setting_duration(float seconds, bool use_default) const
{
	if (!std::isfinite(seconds) || seconds <= 0.0f)
	{
		throw std::invalid_argument("Setting duration requires positive finite seconds.");
	}
	std::wostringstream number;
	number << seconds / 60.0f;
	const std::wstring minutes = format_label_template(this->values.duration_minutes,
		{ { L"{value}", number.str() } });
	return use_default ? format_label_template(this->values.duration_default,
		{ { L"{value}", minutes } }) : minutes;
}

std::wstring MenuText::setting_resolution(int width, int height) const
{
	if (width <= 0 || height <= 0)
	{
		throw std::invalid_argument("Setting resolution requires positive integer width and height.");
	}
	return format_label_template(this->values.resolution,
		{ { L"{width}", std::to_wstring(width) }, { L"{height}", std::to_wstring(height) } });
}

std::wstring MenuText::stage_best_of_mode(int best_of) const
{
	if (best_of != 3 && best_of != 5)
	{
		throw std::invalid_argument("Stage best-of mode requires 3 or 5.");
	}
	return format_label_template(this->stage.best_of_mode, { { L"{best_of}", std::to_wstring(best_of) } });
}

std::wstring MenuText::stage_roster(int players) const
{
	if (players < 1 || players > 4)
	{
		throw std::invalid_argument("Stage roster requires a player count from 1 to 4.");
	}
	return format_label_template(players == 1 ? this->stage.solo_roster : this->stage.versus_roster,
		{ { L"{players}", std::to_wstring(players) } });
}

std::wstring MenuText::stage_navigation(size_t index, size_t total) const
{
	if (total == 0 || index == 0 || index > total)
	{
		throw std::invalid_argument("Stage numbered navigation requires 1 <= index <= total and a positive total.");
	}
	return format_label_template(this->stage.numbered_navigation,
		{ { L"{index}", std::to_wstring(index) }, { L"{total}", std::to_wstring(total) } });
}

std::wstring MenuText::stage_random_hint(size_t count) const
{
	if (count == 0)
	{
		throw std::invalid_argument("Stage Random hint requires a positive eligible stage count.");
	}
	return format_stick_template(this->stage.random_hint, { { L"{count}", std::to_wstring(count) } }, true);
}

std::wstring MenuText::roster_label(int roster_index, int slot) const
{
	return format_label_template(this->labels.roster_row,
		{ { L"{player}", PlayerBadge::label(roster_index) }, { L"{device}", this->device_label(slot) } });
}

std::wstring MenuText::stick_connection(int pad_slot, bool connected) const
{
	if (!input_slots::is_pad(pad_slot))
	{
		throw std::invalid_argument("Stick readout requires a pad slot from 0 to 3.");
	}
	return format_stick_template(connected ? this->stick_readouts.connected : this->stick_readouts.disconnected,
		{ { L"{device}", this->device_label(pad_slot) } }, false);
}

std::wstring MenuText::stick_raw_movement(int value) const
{
	return format_stick_template(this->stick_readouts.raw_movement, { { L"{value}", stick_percent_value(value) } }, false);
}

std::wstring MenuText::stick_movement_response(int value) const
{
	return format_stick_template(this->stick_readouts.movement_response, { { L"{value}", stick_percent_value(value) } }, false);
}

std::wstring MenuText::stick_aim_inactive(int raw_x, int raw_y) const
{
	return format_stick_template(this->stick_readouts.aim_inactive,
		{ { L"{raw_x}", stick_percent_value(raw_x) }, { L"{raw_y}", stick_percent_value(raw_y) } }, true);
}

std::wstring MenuText::stick_aim_active(int raw_x, int raw_y, int aim_x, int aim_y) const
{
	return format_stick_template(this->stick_readouts.aim_active,
		{ { L"{raw_x}", stick_percent_value(raw_x) }, { L"{raw_y}", stick_percent_value(raw_y) },
		{ L"{aim_x}", stick_percent_value(aim_x) }, { L"{aim_y}", stick_percent_value(aim_y) } }, true);
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
	this->music_mix_.pause_gain = read_unit(music_mix, "pause_gain");
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
	if (this->respawn_redraw_.canvas_size != this->figure_.canvas_size)
	{
		// The pen draws over the same overhanging canvas as the figure.
		throw std::runtime_error("respawn_redraw.canvas_size must equal figure.canvas_size.");
	}
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
	signal.pulse = read_unit(countdown_art, "pulse");
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
	check.font("main_menu.stage_select.record_font", this->main_menu_.stage_select.record_font);
	check.font("prompt_bar.font", this->prompt_bar_.font);
	check.font("debug_overlay.font", this->debug_overlay_.font);
	check.font("debug_overlay.station_label.font", this->debug_overlay_.station_label.font);
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
	MenuPaperArtPresentation options_art = main.art;
	options_art.focus_padding = main.setup_focus_padding;
	options_art.shadow_offset = main.setup_shadow_offset;
	const MenuItem home_items[]{ MenuItem::play, MenuItem::options, MenuItem::credits, MenuItem::exit };
	for (size_t index = 0; index < std::size(home_items); ++index)
	{
		check_menu_item(home_items[index], this->menu_.item_font, main.home_position +
			Vector2F(0.0f, main.home_spacing.y * static_cast<float>(index + 2)),
			main.home_box_size, main.home_spacing.y, std::nullopt, main.art, this->menu_.footer_y);
	}
	const MenuItem options_items[]{ MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette,
		MenuItem::controls_audio, MenuItem::restore_defaults, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(options_items); ++index)
	{
		const Vector2F position = main.options_position +
			Vector2F(0.0f, main.options_spacing.y * static_cast<float>(index));
		const std::optional<float> label_width = index < 3 ?
			std::optional<float>(main.options_value_x - position.x - options_art.focus_padding.x) : std::nullopt;
		check_menu_item(options_items[index], this->menu_.item_font, position,
			main.options_box_size, main.options_spacing.y, label_width, options_art, this->menu_.footer_y);
	}
	check_menu_item(MenuItem::back, this->menu_.item_font, main.credits_back_position, main.credits_box_size, 0.0f, std::nullopt,
		main.art, this->menu_.footer_y);
	const PauseMenuPresentation& pause = this->pause_menu_;
	MenuPaperArtPresentation display_art = pause.art;
	display_art.focus_padding = main.setup_focus_padding;
	display_art.shadow_offset = main.setup_shadow_offset;
	if (!std::isfinite(pause.layout.spacing.y) || pause.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.items.resume requires positive finite pause_menu.layout.spacing.y.");
	}
	const MenuItem pause_items[]{ MenuItem::resume, MenuItem::restart, MenuItem::options, MenuItem::quit };
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
	check_menu_item(MenuItem::keep, this->menu_.item_font, pause.layout.position, pause.box_size, pause.layout.spacing.y,
		std::nullopt, display_art, 1080.0f);
	check_menu_item(MenuItem::revert, this->menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		pause.box_size, pause.layout.spacing.y, std::nullopt, display_art, 1080.0f);
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
	const RectangleF setup_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.setup_box_size) / 2.0f,
		main.setup_box_size);
	const float setup_edge = main.art.keyline_width * 3.0f;
	const RectangleF setup_inside(setup_paper.position() + Vector2F(setup_edge),
		setup_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const auto check_setup_paper = [&](const std::string& key, const std::wstring& words,
		const std::string& font, const Vector2F& position, const Vector2F& shadow,
		bool action, float row_spacing)
	{
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), words);
		RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		if (action)
		{
			// Transparent text shadows retain their original navigation extents.
			// Include the entire paper card, its shade and the detached arrow.
			bounds.inflate(main.setup_focus_padding);
			if (std::max(bounds.height + main.setup_shadow_offset,
				main.art.keyline_width * 6.0f) > row_spacing)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					" must fit its setup row including paper padding and shade.");
			}
			const RectangleF arrow(bounds.x - main.art.keyline_width * 7.0f,
				bounds.center().y - main.art.keyline_width * 3.0f,
				main.art.keyline_width * 5.0f, main.art.keyline_width * 6.0f);
			bounds = RectangleF::union_of(arrow, RectangleF::union_of(bounds,
				RectangleF(bounds.position() + Vector2F(main.setup_shadow_offset), bounds.size())));
		}
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !setup_inside.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit main_menu.setup_box_size including its retained shadow bounds and paper ornaments.");
		}
		return bounds;
	};
	float setup_heading_bottom = 0.0f;
	for (const MenuHeading role : { MenuHeading::mode, MenuHeading::players })
	{
		const RectangleF heading = check_setup_paper(
			std::string("menu_text.headings.") + menu_heading_keys[static_cast<size_t>(role)],
			this->menu_text_.heading(role), this->menu_.heading_font, this->menu_.layout.position,
			this->menu_.heading_shadow_offset, false, 0.0f);
		setup_heading_bottom = std::max(setup_heading_bottom, heading.bottom());
	}
	check_setup_paper("menu_text.headings.join_players", this->menu_text_.heading(MenuHeading::join_players),
		this->menu_.heading_font, this->menu_.layout.position, this->menu_.heading_shadow_offset, false, 0.0f);
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
		const float value_x = position.x + spacing.x * 2.0f;
		if (duration_label && (!std::isfinite(value_x) ||
			value_x + main.setup_focus_padding.x + main.setup_shadow_offset >= setup_inside.right()))
		{
			throw std::runtime_error(where +
				" must leave positive duration value space inside main_menu.setup_box_size.");
		}
		const RectangleF card = check_setup_paper(key, text, this->menu_.item_font, position, shadow, true, spacing.y);
		if (row == 2 && card.y < setup_heading_bottom)
		{
			throw std::runtime_error(where + " must leave its setup heading clear of the first focus card.");
		}
		return card;
	};
	const MenuItem mode_items[]{ MenuItem::single_match, MenuItem::best_of_three, MenuItem::best_of_five,
		MenuItem::match_length, MenuItem::back };
	float mode_back_bottom = 0.0f;
	for (size_t index = 0; index < std::size(mode_items); ++index)
	{
		const RectangleF card = check_setup_item(mode_items[index], main.mode_select_spacing, static_cast<int>(index) + 2,
			mode_items[index] == MenuItem::match_length);
		if (mode_items[index] == MenuItem::back) { mode_back_bottom = card.bottom(); }
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
	const RectangleF practice_hint = check_setup_paper(
		"menu_text.solo_practice_hint", this->menu_text_.solo_practice_hint, this->menu_.detail_font,
		this->menu_.layout.position + Vector2F(0.0f, main.mode_select_spacing.y * 7.0f),
		this->menu_.item_shadow_offset, false, 0.0f);
	if (practice_hint.y < mode_back_bottom)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.solo_practice_hint must follow the complete Back focus card and shade.");
	}
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
	const RectangleF confirmation_paper((Vector2F(1920.0f, 1080.0f) - pause.box_size) / 2.0f, pause.box_size);
	RectangleF first_confirmation_action(pause.layout.position,
		fixed_menu_size("menu_text.items.yes", this->menu_text_.item(MenuItem::yes), this->menu_.item_font));
	first_confirmation_action.inflate(pause.art.focus_padding);
	const auto check_confirmation = [&](const char* role, const std::wstring& text, float action_top)
	{
		const std::string key = std::string("menu_text.confirmations.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.detail_font);
		// Confirmation questions remain plain, unscaled and centered on the
		// full-design Pause paper rather than the footer-centered menu paper.
		const RectangleF bounds((1920.0f - size.x) / 2.0f, pause.status_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !confirmation_paper.contains(bounds) ||
			bounds.bottom() > action_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper before the first action's focus padding.");
		}
	};
	check_confirmation("restart", this->menu_text_.confirmations.restart, first_confirmation_action.y);
	check_confirmation("quit", this->menu_text_.confirmations.quit, first_confirmation_action.y);
	check_confirmation("exit", this->menu_text_.confirmations.exit, first_confirmation_action.y);
	check_confirmation("display", this->menu_text_.confirmations.display, pause.layout.position.y - display_art.focus_padding.y);
	const RectangleF end_paper((Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f, end.box_size);
	// The first continuation row starts here regardless of its Session label.
	const float first_end_action_top = end.layout.position.y - end.art.focus_padding.y;
	const auto check_end_heading = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.end.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.heading_font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, end.heading_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !std::isfinite(first_end_action_top) ||
			!end_paper.contains(bounds) || bounds.bottom() > first_end_action_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the End paper before the first action's focus padding.");
		}
	};
	check_end_heading("match_complete", this->menu_text_.end.match_complete);
	check_end_heading("round_complete", this->menu_text_.end.round_complete);
	check_end_heading("series_complete", this->menu_text_.end.series_complete);
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
	const auto check_value = [&](const char* role, const std::wstring& text, const std::string& font,
		const Vector2F& position, const Vector2F& limit, const Vector2F& paper_size,
		const MenuPaperArtPresentation& art, float row_spacing, bool controls_value)
	{
		const std::string key = std::string("menu_text.values.") + role;
		const Vector2F raw = fixed_menu_size(key, text, font);
		if (!std::isfinite(limit.x) || !std::isfinite(limit.y) || limit.x <= 0.0f || limit.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must leave finite positive value width and row height.");
		}
		// Match OptionRow::set_value: reset to the authored font size, fit
		// width first, then height. Supported long values remain valid.
		float scale = 1.0f;
		if (raw.x > limit.x) { scale = limit.x / raw.x; }
		if (raw.y * scale > limit.y) { scale = limit.y / raw.y; }
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must retain finite positive fitted value bounds.");
		}
		const RectangleF bounds(position, fitted);
		check_fixed_paper(key, bounds, paper_size, art.focus_padding);
		if (fitted.y + art.focus_padding.y * 2.0f > row_spacing)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its row including focus padding.");
		}
		if (controls_value && (bounds.right() + art.focus_padding.x > controls.controls_position.x ||
			bounds.bottom() + art.focus_padding.y > controls.status_position.y))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit before the controls help and status including focus padding.");
		}
	};
	const MenuValueText& values = this->menu_text_.values;
	const std::string random_name_key = "menu_text.values.random";
	const Vector2F random_name_size = fixed_menu_size(random_name_key, values.random, this->menu_.item_font);
	const StageSelectPresentation& stage = main.stage_select;
	const Vector2F name_shadow = this->menu_.item_shadow_offset;
	const float name_bottom = stage.preview_position.y - this->menu_.rule_height;
	if (!std::isfinite(name_shadow.x) || !std::isfinite(name_shadow.y) ||
		!std::isfinite(stage.preview_size.x) || stage.preview_size.x <= 0.0f ||
		!std::isfinite(name_bottom) || name_bottom <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ + ": " + random_name_key +
			" must have a finite shadow, positive preview width and positive name area.");
	}
	const RectangleF random_name_union = RectangleF::union_of(RectangleF(Vector2F::ZERO, random_name_size),
		RectangleF(name_shadow, random_name_size));
	if (!std::isfinite(random_name_union.x) || !std::isfinite(random_name_union.y) ||
		!std::isfinite(random_name_union.width) || !std::isfinite(random_name_union.height) ||
		random_name_union.width <= 0.0f || random_name_union.height <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ + ": " + random_name_key +
			" must have finite positive stage-name bounds including its shadow.");
	}
	// Match the stage page's width-only fit, including signed shadow offsets.
	const float random_name_scale = std::min(1.0f, stage.preview_size.x / random_name_union.width);
	const Vector2F fitted_name_size = random_name_size * random_name_scale;
	const RectangleF fitted_random_name = RectangleF::union_of(RectangleF(stage.name_position, fitted_name_size),
		RectangleF(stage.name_position + name_shadow * random_name_scale, fitted_name_size));
	if (!std::isfinite(random_name_scale) || random_name_scale <= 0.0f ||
		!std::isfinite(fitted_random_name.x) || !std::isfinite(fitted_random_name.y) ||
		!std::isfinite(fitted_random_name.width) || !std::isfinite(fitted_random_name.height) ||
		fitted_random_name.width <= 0.0f || fitted_random_name.height <= 0.0f ||
		!RectangleF(0.0f, 0.0f, 1920.0f, name_bottom).contains(fitted_random_name))
	{
		throw std::runtime_error(this->source_path_ + ": " + random_name_key +
			" must fit its stage-name design area above the preview edge including its scaled shadow.");
	}
	const RectangleF stage_briefing(stage.briefing.position, stage.briefing.size);
	const Vector2F stage_info = stage.briefing.position + stage.briefing_padding;
	const Vector2F stage_status_position = stage_info + stage.briefing.spacing * 6.0f;
	const float stage_hint_top = (stage_info + stage.briefing.spacing * 7.0f).y;
	const auto check_stage_status = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.detail_font);
		const Vector2F shadow = this->menu_.detail_shadow_offset;
		const RectangleF bounds = RectangleF::union_of(RectangleF(stage_status_position, size),
			RectangleF(stage_status_position + shadow, size));
		// Status text keeps its unscaled font. Padding locates its origin;
		// unlike the following hint, it has no inset-width fitting rule.
		if (!std::isfinite(stage_briefing.x) || !std::isfinite(stage_briefing.y) ||
			!std::isfinite(stage_briefing.width) || !std::isfinite(stage_briefing.height) ||
			stage_briefing.width <= 0.0f || stage_briefing.height <= 0.0f ||
			!std::isfinite(stage_hint_top) || !std::isfinite(shadow.x) || !std::isfinite(shadow.y) ||
			!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !stage_briefing.contains(bounds) ||
			!RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(bounds) || bounds.bottom() > stage_hint_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the briefing panel and design area before the authored hint row origin, including its shadow.");
		}
	};
	check_stage_status("browse_status", this->menu_text_.stage.browse_status);
	check_stage_status("locked_status", this->menu_text_.stage.locked_status);
	const auto check_stage_briefing = [&](const char* role, const std::wstring& text, const std::string& font,
		float line, float next_line)
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, font);
		const Vector2F position = stage_info + stage.briefing.spacing * line;
		const Vector2F shadow = this->menu_.detail_shadow_offset;
		const RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		const float next_top = (stage_info + stage.briefing.spacing * next_line).y;
		// These labels and authored blocks retain the briefing's unscaled font.
		// Their text is not clipped to the panel's width, including the shipped
		// solo objective, which extends beyond its right edge.
		if (!std::isfinite(next_top) || !std::isfinite(shadow.x) || !std::isfinite(shadow.y) ||
			!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || bounds.y < stage_briefing.y ||
			bounds.bottom() > stage_briefing.bottom() ||
			!RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(bounds) || bounds.bottom() > next_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the design area and briefing vertical span before the next authored row origin, including its shadow.");
		}
		return bounds;
	};
	check_stage_briefing("briefing_label", this->menu_text_.stage.briefing_label, this->menu_.brand_font, 0.0f, 1.0f);
	check_stage_briefing("goal", this->menu_text_.stage.goal, this->menu_.brand_font, 3.0f, 4.0f);
	check_stage_briefing("solo_objective", this->menu_text_.stage.solo_objective, this->menu_.brand_font, 4.0f, 6.0f);
	check_stage_briefing("versus_objective", this->menu_text_.stage.versus_objective, this->menu_.brand_font, 4.0f, 6.0f);
	check_stage_briefing("solo_mode", this->menu_text_.stage.solo_mode, this->menu_.detail_font, 1.0f, 2.0f);
	check_stage_briefing("single_match_mode", this->menu_text_.stage.single_match_mode, this->menu_.detail_font, 1.0f, 2.0f);
	const auto check_stage_summary = [&](const char* role, const std::wstring& text, const std::string& font,
		float line, float next_line)
	{
		RectangleF bounds = check_stage_briefing(role, text, font, line, next_line);
		// The page expands the briefing artwork around unscaled copy rather
		// than fitting copy to its authored width. That expanded card must
		// still fit the outer paper and remain separate from the preview.
		const float edge = main.art.keyline_width * 3.0f;
		const float inset = edge + main.setup_shadow_offset;
		bounds.inflate(inset);
		const RectangleF card = RectangleF::union_of(stage_briefing, bounds);
		const RectangleF outer((Vector2F(1920.0f, this->menu_.footer_y) - main.stage_box_size) / 2.0f,
			main.stage_box_size);
		const RectangleF inside(outer.position() + Vector2F(edge),
			outer.size() - Vector2F(edge * 2.0f + main.setup_shadow_offset));
		RectangleF preview(stage.preview_position, stage.preview_size);
		preview.inflate(this->menu_.rule_height);
		const auto finite_rectangle = [](const RectangleF& rectangle)
		{
			return std::isfinite(rectangle.x) && std::isfinite(rectangle.y) &&
				std::isfinite(rectangle.width) && std::isfinite(rectangle.height) &&
				std::isfinite(rectangle.right()) && std::isfinite(rectangle.bottom()) &&
				rectangle.width > 0.0f && rectangle.height > 0.0f;
		};
		if (!std::isfinite(edge) || !std::isfinite(inset) || !finite_rectangle(card) ||
			!finite_rectangle(inside) || !finite_rectangle(preview) || !inside.contains(card) ||
			(card.x < preview.right() && card.right() > preview.x &&
				card.y < preview.bottom() && card.bottom() > preview.y))
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.stage." + role +
				" must keep the expanded briefing card inside the Stage paper and separate from the preview edge, including its retained artwork and shadow.");
		}
	};
	// Stage records share the navigation row, right-aligned to the preview,
	// and the page shrinks a line to the room beside the arena counter. The
	// copy must draw in its font and the row must sit below the preview edge
	// and above the footer.
	const RectangleF record_preview(stage.preview_position, stage.preview_size);
	for (const std::wstring* label : { &this->menu_text_.records.best_win, &this->menu_text_.records.best_coverage })
	{
		const std::wstring line = *label + L" 100.0% / 99 " + this->menu_text_.records.played;
		const Vector2F size = fixed_menu_size("menu_text.records", line, stage.record_font) +
			this->menu_.detail_shadow_offset;
		if (stage.record_y < record_preview.bottom() + this->menu_.rule_height ||
			stage.record_y + size.y > this->menu_.footer_y)
		{
			throw std::runtime_error(this->source_path_ +
				": main_menu.stage_select.record_y and record_font must place menu_text.records "
				"below the preview edge and above the footer.");
		}
	}
	const ResultsMenuPresentation& results = this->results_menu_;
	const RectangleF results_panel((Vector2F(1920.0f, 1080.0f) - results.box_size) / 2.0f, results.box_size);
	const float results_fill_right = (1920.0f + results.fill_box_size.x) / 2.0f;
	const float results_fill_bottom = results_panel.y + results.fill_box_y_offset + results.fill_box_size.y;
	const RectangleF contribution_heading(results.contribution_position, fixed_menu_size(
		"menu_text.results.contribution_heading", this->menu_text_.results.contribution_heading,
		results.contribution_font));
	for (const std::wstring* label : { &this->menu_text_.records.new_best, &this->menu_text_.records.stage_best })
	{
		const Vector2F size = fixed_menu_size("menu_text.records", *label + L" 100.0%", results.contribution_font);
		const RectangleF bounds(Vector2F(results_fill_right - size.x, results.record_y), size);
		if (!results_panel.contains(bounds) || bounds.y < results_fill_bottom ||
			bounds.bottom() > results.proceed_y || bounds.x <= contribution_heading.right())
		{
			throw std::runtime_error(this->source_path_ +
				": results_menu.record_y and menu_text.records must fit the panel below the score bar, "
				"above the prompt and to the right of the contribution heading.");
		}
	}
	const auto finite_stage_rectangle = [](const RectangleF& bounds)
	{
		return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
			std::isfinite(bounds.width) && std::isfinite(bounds.height) &&
			std::isfinite(bounds.right()) && std::isfinite(bounds.bottom()) &&
			bounds.width > 0.0f && bounds.height > 0.0f;
	};
	const auto stage_rectangles_overlap = [](const RectangleF& first, const RectangleF& second)
	{
		return first.x < second.right() && first.right() > second.x &&
			first.y < second.bottom() && first.bottom() > second.y;
	};
	const std::string navigation_key = "menu_text.stage.random_navigation";
	const Vector2F navigation_size = fixed_menu_size(navigation_key, this->menu_text_.stage.random_navigation,
		this->menu_.detail_font);
	const Vector2F stage_detail_shadow = this->menu_.detail_shadow_offset;
	const RectangleF navigation_bounds = RectangleF::union_of(RectangleF(stage.navigation_position, navigation_size),
		RectangleF(stage.navigation_position + stage_detail_shadow, navigation_size));
	RectangleF preview_edge(stage.preview_position, stage.preview_size);
	preview_edge.inflate(this->menu_.rule_height);
	if (!std::isfinite(stage_detail_shadow.x) || !std::isfinite(stage_detail_shadow.y) ||
		!std::isfinite(this->menu_.footer_y) || !finite_stage_rectangle(navigation_bounds) ||
		!finite_stage_rectangle(preview_edge) || !finite_stage_rectangle(stage_briefing) ||
		!RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(navigation_bounds) ||
		navigation_bounds.bottom() > this->menu_.footer_y ||
		stage_rectangles_overlap(navigation_bounds, preview_edge) ||
		stage_rectangles_overlap(navigation_bounds, stage_briefing))
	{
		throw std::runtime_error(this->source_path_ + ": " + navigation_key +
			" must fit unscaled inside the design area before the footer without overlapping the preview edge or briefing panel, including its shadow.");
	}
	const Vector2F stage_hint_position = stage_info + stage.briefing.spacing * 7.0f;
	const float stage_hint_width = stage.briefing.size.x - 2.0f * stage.briefing_padding.x;
	const auto check_stage_hint = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.brand_font);
		const RectangleF raw = RectangleF::union_of(RectangleF(Vector2F::ZERO, size),
			RectangleF(stage_detail_shadow, size));
		if (!std::isfinite(stage_hint_width) || stage_hint_width <= 0.0f || !finite_stage_rectangle(raw))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must have a finite positive hint width and text bounds including its shadow.");
		}
		// Match refresh: measure at scale one and fit only the full union's width.
		const float scale = std::min(1.0f, stage_hint_width / raw.width);
		const Vector2F fitted_size = size * scale;
		const RectangleF bounds = RectangleF::union_of(RectangleF(stage_hint_position, fitted_size),
			RectangleF(stage_hint_position + stage_detail_shadow * scale, fitted_size));
		if (!std::isfinite(scale) || scale <= 0.0f || !finite_stage_rectangle(bounds) ||
			!stage_briefing.contains(bounds) || !RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(bounds) ||
			bounds.bottom() > this->menu_.footer_y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its briefing panel and design area before the footer using width-only fitting, including its scaled shadow.");
		}
	};
	check_stage_hint("locked_hint", this->menu_text_.stage.locked_hint);
	check_stage_hint("browse_hint", this->menu_text_.stage.browse_hint);
	const Vector2F options_value_limit((1920.0f + main.options_box_size.x) / 2.0f -
		options_art.shadow_offset - options_art.focus_padding.x - main.options_value_x,
		main.options_spacing.y - options_art.focus_padding.y * 2.0f);
	const auto options_value_position = [&](int row)
	{
		return Vector2F(main.options_value_x, main.options_position.y + main.options_spacing.y * static_cast<float>(row));
	};
	check_value("on", values.on, this->menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("off", values.off, this->menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("random", values.random, this->menu_.item_font, options_value_position(2), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	const Vector2F controls_value_limit(controls.controls_position.x - controls.art.focus_padding.x * 2.0f -
		controls.art.shadow_offset - controls.layout.position.x - controls.layout.spacing.x,
		controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
	for (const int row : { 3, 4, 5 })
	{
		const Vector2F position = controls.layout.position +
			Vector2F(controls.layout.spacing.x, controls.layout.spacing.y * static_cast<float>(row));
		check_value("on", values.on, controls.item_font, position, controls_value_limit,
			controls.box_size, controls.art, controls.layout.spacing.y, true);
		check_value("off", values.off, controls.item_font, position, controls_value_limit,
			controls.box_size, controls.art, controls.layout.spacing.y, true);
	}
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
	const RectangleF help_area(controls.controls_position, controls.controls_size);
	check_fixed_paper("menu_text.help.controls", help_area, controls.box_size, Vector2F::ZERO);
	const auto check_help_fit = [&](const std::string& key, const std::wstring& text,
		const Vector2F& position, const Vector2F& available)
	{
		const Vector2F raw = fixed_menu_size(key, text, controls.help_font);
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			!std::isfinite(available.x) || !std::isfinite(available.y) || available.x <= 0.0f || available.y <= 0.0f ||
			!help_area.contains(RectangleF(position, available)))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must have a finite positive fitting area inside the controls help region.");
		}
		// Preserve the two pages' existing uniform fit. The available region
		// bounds it by construction; supported long blocks need no length cap.
		const float scale = std::min({ 1.0f, available.x / raw.x, available.y / raw.y });
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must retain finite positive text bounds after fitting.");
		}
		return RectangleF(position, fitted);
	};
	const RectangleF controls_help = check_help_fit("menu_text.help.controls", this->menu_text_.help.controls,
		controls.controls_position, controls.controls_size);
	const StickPreviewPresentation& preview = controls.stick_preview;
	const RectangleF pad_association = check_help_fit("menu_text.help.pad_association", this->menu_text_.help.pad_association,
		preview.association_position, help_area.bottom_right() - preview.association_position);
	if (preview.association_position.y < preview.aim.bottom())
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.help.pad_association must begin below the aim preview.");
	}
	const Vector2F caption_size = fixed_menu_size("menu_text.help.aim_cutoff",
		this->menu_text_.help.aim_cutoff, controls.help_font);
	const float probe_height = render.measure_text(render.resolve_sprite_font(controls.help_font), L"Move").y;
	const RectangleF caption(preview.aim.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y), caption_size);
	if (!std::isfinite(probe_height) || probe_height <= 0.0f || !help_area.contains(caption) ||
		caption.y < preview.movement_response.bottom() || caption.bottom() > preview.aim.y)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.help.aim_cutoff must fit unscaled inside the controls help region between the movement and aim previews.");
	}
	const auto padded_back = [&](const Vector2F& position, const std::string& font,
		const MenuPaperArtPresentation& art)
	{
		RectangleF bounds(position, fixed_menu_size("menu_text.items.back", this->menu_text_.item(MenuItem::back), font));
		bounds.inflate(art.focus_padding);
		return bounds;
	};
	const RectangleF options_back = padded_back(main.options_position + Vector2F(0.0f, main.options_spacing.y * 6.0f),
		this->menu_.item_font, options_art);
	const RectangleF controls_back = padded_back(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 7.0f),
		controls.item_font, controls.art);
	const RectangleF sticks_back = padded_back(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 4.0f),
		controls.item_font, controls.art);
	struct StatusMessage { const char* key; const std::wstring& text; };
	const auto check_status = [&](const StatusMessage& message, const std::string& font,
		const Vector2F& position, const Vector2F& paper_size, const MenuPaperArtPresentation& art,
		std::initializer_list<RectangleF> obstacles)
	{
		const std::string key = std::string("menu_text.status.") + message.key;
		const Vector2F raw = fixed_menu_size(key, message.text, font);
		const float available = (1920.0f + paper_size.x) / 2.0f - art.shadow_offset -
			art.focus_padding.x - position.x;
		if (!std::isfinite(available) || available <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key + " must leave finite positive status width.");
		}
		// All three pages reset to the authored font size, then fit width only.
		const float scale = std::min(1.0f, available / raw.x);
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key + " must retain finite positive fitted status bounds.");
		}
		const RectangleF bounds(position, fitted);
		check_fixed_paper(key, bounds, paper_size, Vector2F::ZERO);
		for (const RectangleF& obstacle : obstacles)
		{
			// Touching edges are allowed. Options' status and Back deliberately
			// share vertical space while remaining horizontally separate.
			if (bounds.x < obstacle.right() && bounds.right() > obstacle.x &&
				bounds.y < obstacle.bottom() && bounds.bottom() > obstacle.y)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					" must not overlap the Back card, help text or stick previews.");
			}
		}
	};
	const MenuStatusText& status_text = this->menu_text_.status;
	const StatusMessage options_statuses[]{ { "options_pending", status_text.options_pending },
		{ "options_saved", status_text.options_saved }, { "options_save_failed", status_text.options_save_failed },
		{ "options_reverted", status_text.options_reverted }, { "options_display_failed", status_text.options_display_failed },
		{ "options_rollback_failed", status_text.options_rollback_failed } };
	for (const StatusMessage& message : options_statuses)
	{
		check_status(message, this->prompt_bar_.font, main.options_status_position, main.options_box_size,
			options_art, { options_back });
	}
	const StatusMessage controls_statuses[]{ { "controls_pending", status_text.controls_pending },
		{ "controls_saved", status_text.controls_saved }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : controls_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { controls_back, controls_help });
	}
	const StatusMessage sticks_statuses[]{ { "sticks_pending", status_text.sticks_pending },
		{ "sticks_saved", status_text.sticks_saved }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : sticks_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { sticks_back, pad_association, preview.raw_movement, preview.movement_response, preview.aim, caption });
	}
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
	check.frame("figure.rest_arms_frame", this->figure_.sprite_sheet, this->figure_.rest_arms_frame);
	check.frame("figure.rest_arms_paint_frame", this->figure_.sprite_sheet,
		this->figure_.rest_arms_paint_frame);
	check.frame("figure.sleeves_frame", this->figure_.sprite_sheet, this->figure_.sleeves_frame);
	check.frame("figure.sleeves_paint_frame", this->figure_.sprite_sheet,
		this->figure_.sleeves_paint_frame);
	check.frame("tool_motion.mist_tail.fill_frame", this->tool_motion_.mist_sheet,
		this->tool_motion_.mist_frame);
	check.frame("respawn_redraw.pen_frame", this->respawn_redraw_.sprite_sheet,
		this->respawn_redraw_.pen_frame);
	check.frame("opening.fill_frame", this->opening_.sprite_sheet,
		this->opening_.fill_frame);
	check.sheet("opening.palette_sheet", this->opening_.palette_sheet);
	const SpriteSheet* palette_sheet = render.sprite_sheet(this->opening_.palette_sheet);
	for (size_t index = 0; index < this->opening_.palette_frames.size(); ++index)
	{
		const std::string key = "opening.palette_frames[" + std::to_string(index) + "]";
		const std::string& name = this->opening_.palette_frames[index];
		check.frame(key, this->opening_.palette_sheet, name);
		const SpriteFrame& swatch = palette_sheet->sprite_frame(palette_sheet->resolve_sprite_frame(name));
		const RectangleI& source = swatch.source_rectangle();
		if (source.width <= 0 || source.height <= 0 || source.width != source.height ||
			swatch.origin() != Vector2F::ZERO)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must name a positive square swatch with zero authored origin.");
		}
	}
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
	check.wave("menu_style.sounds.direction.name", bank, this->menu_.direction_sound.name);
	check.wave("menu_style.sounds.confirm.name", bank, this->menu_.confirm_sound.name);
	check.wave("menu_style.sounds.cancel.name", bank, this->menu_.cancel_sound.name);
	check.wave("main_menu.sounds.error.name", bank, this->main_menu_.error_sound.name);
	check.wave("main_menu.sounds.ready.name", bank, this->main_menu_.ready_sound.name);
	check.wave("main_menu.sounds.unready.name", bank, this->main_menu_.unready_sound.name);
	check.wave("controls_settings.sounds.volume_preview.name", bank, this->controls_settings_.volume_preview_sound.name);
	check.effect("main_menu.music.name", bank, this->main_menu_.music.name);
	check.wave("pause_menu.window_open_sound.name", bank, this->pause_menu_.window_open_sound.name);
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
	// Preserve the existing diagnostics when a shared font or shadow also
	// invalidates an earlier page, then check Team's additional paper bounds.
	const RectangleF team_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.team_box_size) / 2.0f,
		main.team_box_size);
	const RectangleF team_inside(team_paper.position() + Vector2F(setup_edge),
		team_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const auto check_team_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, const Vector2F& shadow)
	{
		drawable_text::check_font_can_draw(&render, font, words, this->source_path_ + ": " + key);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), words);
		const RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !team_inside.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit main_menu.team_box_size including its retained shadow bounds.");
		}
		return bounds;
	};
	const RectangleF team_heading = check_team_text("menu_text.headings.teams",
		this->menu_text_.heading(MenuHeading::teams), this->menu_.heading_font,
		this->menu_.layout.position, this->menu_.heading_shadow_offset);
	const RectangleF team_warning = check_team_text(
		"menu_text.connection.teams_required at main_menu.team_select.warning_position",
		this->menu_text_.connection.teams_required, this->menu_.detail_font,
		main.team_warning_position, this->menu_.detail_shadow_offset);
	if (team_warning.y < team_heading.bottom())
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.connection.teams_required at main_menu.team_select.warning_position must follow the Team Select heading.");
	}
	const RectangleF weapon_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.weapon_box_size) / 2.0f,
		main.weapon_box_size);
	const RectangleF weapon_inside(weapon_paper.position() + Vector2F(setup_edge),
		weapon_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const Vector2F weapon_heading_size = render.measure_text(render.resolve_sprite_font(this->menu_.heading_font),
		this->menu_text_.heading(MenuHeading::weapons));
	const RectangleF weapon_heading = RectangleF::union_of(
		RectangleF(this->menu_.layout.position, weapon_heading_size),
		RectangleF(this->menu_.layout.position + this->menu_.heading_shadow_offset, weapon_heading_size));
	if (!std::isfinite(weapon_heading.x) || !std::isfinite(weapon_heading.y) ||
		!std::isfinite(weapon_heading.width) || !std::isfinite(weapon_heading.height) ||
		weapon_heading.width <= 0.0f || weapon_heading.height <= 0.0f || !weapon_inside.contains(weapon_heading))
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.headings.weapons must fit main_menu.weapon_box_size including its retained shadow bounds.");
	}
	const RectangleF stage_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.stage_box_size) / 2.0f,
		main.stage_box_size);
	const RectangleF stage_inside(stage_paper.position() + Vector2F(setup_edge),
		stage_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const Vector2F stage_heading_size = render.measure_text(render.resolve_sprite_font(this->menu_.heading_font),
		this->menu_text_.heading(MenuHeading::arena));
	const RectangleF stage_heading = RectangleF::union_of(
		RectangleF(this->menu_.layout.position, stage_heading_size),
		RectangleF(this->menu_.layout.position + this->menu_.heading_shadow_offset, stage_heading_size));
	if (!std::isfinite(stage_heading.x) || !std::isfinite(stage_heading.y) ||
		!std::isfinite(stage_heading.width) || !std::isfinite(stage_heading.height) ||
		stage_heading.width <= 0.0f || stage_heading.height <= 0.0f || !stage_inside.contains(stage_heading))
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.headings.arena must fit main_menu.stage_box_size including its retained shadow bounds.");
	}
	// Home's Exit confirmation reuses Pause paper/layout, with the existing
	// Exit label rendered once more in the actual heading font.
	const Vector2F exit_heading_size = fixed_menu_size("menu_text.items.exit",
		this->menu_text_.item(MenuItem::exit), this->menu_.heading_font);
	const RectangleF exit_heading((1920.0f - exit_heading_size.x) / 2.0f, pause.heading_y,
		exit_heading_size.x, exit_heading_size.y);
	if (!std::isfinite(exit_heading.x) || !std::isfinite(exit_heading.y) ||
		!confirmation_paper.contains(exit_heading) || exit_heading.bottom() > pause.status_y)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.items.exit must fit unscaled inside the Pause paper before the Exit confirmation question.");
	}
	const Vector2F display_heading_size = fixed_menu_size("menu_text.headings.options",
		this->menu_text_.heading(MenuHeading::options), this->menu_.heading_font);
	const RectangleF display_heading((1920.0f - display_heading_size.x) / 2.0f, pause.heading_y,
		display_heading_size.x, display_heading_size.y);
	if (!std::isfinite(display_heading.x) || !std::isfinite(display_heading.y) ||
		!confirmation_paper.contains(display_heading) || display_heading.bottom() > pause.status_y)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.headings.options must fit unscaled inside the Pause paper before the display confirmation question.");
	}
	RectangleF revert_card(pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		fixed_menu_size("menu_text.items.revert", this->menu_text_.item(MenuItem::revert), this->menu_.item_font));
	revert_card.inflate(display_art.focus_padding);
	const float countdown_y = pause.layout.position.y + pause.layout.spacing.y * 2.0f;
	for (int seconds = 0; seconds <= static_cast<int>(std::ceil(main.display_trial_seconds)); ++seconds)
	{
		const std::string key = "menu_text.display_trial_countdown";
		const Vector2F size = fixed_menu_size(key, this->menu_text_.display_countdown(seconds), this->menu_.detail_font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, countdown_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !confirmation_paper.contains(bounds) ||
			bounds.y < revert_card.bottom())
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper after the Revert focus card for every countdown value.");
		}
	}
	// Display combines compact Main geometry with Pause's retained keyline.
	// Keep the same text-to-face clearance that each authored paper requires.
	if (display_art.focus_padding.x <= display_art.shadow_offset + display_art.keyline_width ||
		display_art.focus_padding.y <= display_art.shadow_offset + display_art.keyline_width)
	{
		throw std::runtime_error(this->source_path_ +
			": main_menu.setup_focus_padding must exceed main_menu.setup_shadow_offset + "
			"pause_menu.art.keyline_width on both axes for the Display confirmation paper.");
	}
	const std::string palette_timing_key = "menu_text.options_palette_timing";
	const Vector2F palette_timing_size = fixed_menu_size(palette_timing_key,
		this->menu_text_.options_palette_timing, this->menu_.detail_font);
	const RectangleF palette_timing(Vector2F(main.options_value_x, main.options_heading_position.y), palette_timing_size);
	check_fixed_paper(palette_timing_key, palette_timing, main.options_box_size, Vector2F::ZERO);
	const RectangleF options_heading(main.options_heading_position,
		fixed_menu_size("menu_text.headings.options", this->menu_text_.heading(MenuHeading::options), this->menu_.heading_font));
	if (palette_timing.x < options_heading.right() ||
		palette_timing.bottom() > main.options_position.y - options_art.focus_padding.y)
	{
		throw std::runtime_error(this->source_path_ + ": " + palette_timing_key +
			" must fit unscaled beside the Options heading and above the first action's focus padding.");
	}
	// Check these additional roles after the established pages so a shared font
	// or geometry error retains its earlier diagnostic wherever possible.
	const auto label_key = [](int slot)
	{
		return std::string("menu_text.labels.roster_row with menu_text.labels.") +
			(input_slots::is_pad(slot) ? "pad" : input_slots::is_keyboard_and_mouse(slot) ? "keyboard_mouse" : "waiting");
	};
	const auto label_size = [&](const std::string& key, const std::wstring& words, const std::string& font_name)
	{
		const std::string where = this->source_path_ + ": " + key;
		drawable_text::check_font_can_draw(&render, font_name, words, where);
		const Font& font = *render.font(render.resolve_sprite_font(font_name));
		LabelPen pen;
		append_label_text(pen, font, words, where);
		if (pen.extent.x <= 0.0f || pen.extent.y <= 0.0f)
		{
			throw std::runtime_error(where + " must retain finite positive composed label bounds.");
		}
		return pen.extent;
	};
	const MenuConnectionText& connection = this->menu_text_.connection;
	const std::string reconnect_key = "menu_text.connection.join_reconnect_suffix";
	const RectangleF join_heading = check_setup_paper("menu_text.headings.join_players",
		this->menu_text_.heading(MenuHeading::join_players), this->menu_.heading_font,
		this->menu_.layout.position, this->menu_.heading_shadow_offset, false, 0.0f);
	float join_previous_bottom = join_heading.bottom();
	std::vector<std::string> join_previous_keys{ "menu_text.headings.join_players" };
	const auto previous_join_context = [&]
	{
		std::string context;
		for (const std::string& key : join_previous_keys)
		{
			if (!context.empty()) { context += " or "; }
			context += key;
		}
		return context;
	};
	for (int player = 0; player < 4; ++player)
	{
		const Vector2F position = this->menu_.layout.position +
			Vector2F(0.0f, main.player_count_spacing.y * static_cast<float>(player + 2));
		float row_bottom = join_previous_bottom;
		std::vector<std::string> row_bottom_keys = join_previous_keys;
		for (int slot = -1; slot < input_slots::count; ++slot)
		{
			const std::wstring label = this->menu_text_.roster_label(player, slot);
			const auto check_join_row = [&](const std::wstring& words, bool reconnect)
			{
				const std::string key = label_key(slot) + " at Join" + (reconnect ? " with " + reconnect_key : "");
				std::ignore = label_size(key, words, this->menu_.item_font);
				RectangleF bounds = check_setup_paper(key, words, this->menu_.item_font,
					position, this->menu_.item_shadow_offset, false, 0.0f);
				// Match Join's retained acknowledgement strip beside the original
				// text bounds, without adding focus-card padding to these rows.
				const RectangleF accent(bounds.x - this->menu_.focus_padding.x, bounds.y,
					std::max(0.0f, this->menu_.focus_padding.x), bounds.height);
				bounds = RectangleF::union_of(bounds, accent);
				if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
					!std::isfinite(bounds.width) || !std::isfinite(bounds.height) || !setup_inside.contains(bounds))
				{
					throw std::runtime_error(this->source_path_ + ": " + key +
						" must fit every composed Join player/device row inside the setup paper including its retained shadow and acknowledgement bounds.");
				}
				if (bounds.y < join_previous_bottom)
				{
					throw std::runtime_error(this->source_path_ + ": " + key +
						" requires Join player rows to follow the heading and each other without overlap"
						" (preceding envelope contributions from " + previous_join_context() + ").");
				}
				if (bounds.bottom() > row_bottom)
				{
					row_bottom_keys = { key };
				}
				else if (bounds.bottom() == row_bottom &&
					std::find(row_bottom_keys.begin(), row_bottom_keys.end(), key) == row_bottom_keys.end())
				{
					row_bottom_keys.push_back(key);
				}
				row_bottom = std::max(row_bottom, bounds.bottom());
			};
			check_join_row(label, false);
			// The shared keyboard/mouse input is always connected. Only a
			// reserved pad can actually display the reconnect suffix.
			if (input_slots::is_pad(slot)) { check_join_row(label + connection.join_reconnect_suffix, true); }
		}
		join_previous_bottom = row_bottom;
		join_previous_keys = std::move(row_bottom_keys);
	}
	const auto check_join_status = [&](const char* role, const std::wstring& words)
	{
		const std::string key = std::string("menu_text.connection.") + role;
		std::ignore = fixed_menu_size(key, words, this->menu_.detail_font);
		const RectangleF bounds = check_setup_paper(key, words, this->menu_.detail_font,
			this->menu_.layout.position + Vector2F(0.0f, main.player_count_spacing.y * 7.0f),
			this->menu_.detail_shadow_offset, false, 0.0f);
		if (bounds.y < join_previous_bottom)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must follow all four Join player rows without overlap"
				" (preceding envelope contributions from " + previous_join_context() + ").");
		}
	};
	check_join_status("join_ready", connection.join_ready);
	check_join_status("join_waiting", connection.join_waiting);
	const auto check_pause_connection = [&](const char* role, const std::wstring& words, bool heading)
	{
		const std::string key = std::string("menu_text.connection.") + role;
		const Vector2F size = fixed_menu_size(key, words,
			heading ? this->menu_.heading_font : this->menu_.detail_font);
		// Pause uses plain UiText, centered on the full-design paper. It has
		// neither Main's footer centering nor retained text-shadow bounds.
		const RectangleF bounds((1920.0f - size.x) / 2.0f,
			heading ? pause.heading_y : pause.status_y, size.x, size.y);
		const float bottom = heading ? pause.status_y : pause.layout.position.y - pause.art.focus_padding.y;
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !std::isfinite(bottom) ||
			!confirmation_paper.contains(bounds) || bounds.bottom() > bottom)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper in heading, status, first-action order including the action's focus padding.");
		}
	};
	check_pause_connection("pause_title", connection.pause_title, true);
	for (int number = 1; number <= 4; ++number)
	{
		check_pause_connection("pause_player_title", this->menu_text_.pause_title_for_player(number), true);
	}
	check_pause_connection("controller_lost", connection.controller_lost, true);
	check_pause_connection("all_connected", connection.all_connected, false);
	const Font& stick_font = *render.font(render.resolve_sprite_font(controls.help_font));
	const auto stick_numeric_size = [&](const char* role, const std::wstring& text)
	{
		const std::string where = this->source_path_ + ": menu_text.stick_readouts." + role;
		drawable_text::check_font_can_draw(&render, controls.help_font, L"-0123456789", where);
		struct Pen
		{
			float x = 0.0f;
			float y = 0.0f;
			Vector2F extent = Vector2F::ZERO;
		};
		const auto walk = [&](Pen& pen, std::wstring_view words)
		{
			// Match Font's pen order, including accumulated bearings, the clamp
			// at zero, whitespace suppression and glyph overhang beyond a line.
			for (const wchar_t character : words)
			{
				if (character == L'\n')
				{
					pen.x = 0.0f;
					pen.y += stick_font.line_spacing();
					continue;
				}
				const Glyph& glyph = stick_font.drawn(static_cast<char32_t>(character));
				if (!std::isfinite(glyph.x_offset) || !std::isfinite(glyph.y_offset) ||
					!std::isfinite(glyph.x_advance))
				{
					throw std::runtime_error(where + " requires finite font glyph metrics.");
				}
				const float width = static_cast<float>(glyph.subrect.width);
				pen.x += glyph.x_offset;
				if (pen.x < 0.0f) { pen.x = 0.0f; }
				if (character != L' ' || glyph.subrect.width > 1 || glyph.subrect.height > 1)
				{
					const float height = character == L' ' ? stick_font.line_spacing() :
						std::max(stick_font.line_spacing(), static_cast<float>(glyph.subrect.height) + glyph.y_offset);
					pen.extent.x = std::max(pen.extent.x, pen.x + width);
					pen.extent.y = std::max(pen.extent.y, pen.y + height);
				}
				pen.x += width + glyph.x_advance;
			}
		};
		if (!std::isfinite(stick_font.line_spacing()) || stick_font.line_spacing() <= 0.0f)
		{
			throw std::runtime_error(where + " requires finite positive font line spacing.");
		}
		Pen pen;
		size_t begin = 0;
		for (;;)
		{
			const size_t token = text.find(L'{', begin);
			const std::wstring_view literal = std::wstring_view(text).substr(begin,
				token == std::wstring::npos ? token : token - begin);
			drawable_text::check_font_can_draw(&render, controls.help_font, literal, where);
			walk(pen, literal);
			if (token == std::wstring::npos) { break; }
			// Every incoming-pen transform is monotone. The maximum reachable
			// pen suffices for the next token, but its greatest ink extent may
			// come from a different value, so retain both maxima independently.
			// Tokens contain no LF; their line position is independent of value.
			float maximum_pen = -std::numeric_limits<float>::infinity();
			Vector2F maximum_extent = pen.extent;
			for (int value = -100; value <= 100; ++value)
			{
				Pen candidate = pen;
				walk(candidate, std::to_wstring(value));
				maximum_pen = std::max(maximum_pen, candidate.x);
				maximum_extent.x = std::max(maximum_extent.x, candidate.extent.x);
				maximum_extent.y = std::max(maximum_extent.y, candidate.extent.y);
			}
			pen.x = maximum_pen;
			pen.extent = maximum_extent;
			// The required template was checked while its source key was read.
			begin = text.find(L'}', token) + 1;
		}
		if (!std::isfinite(pen.x) || !std::isfinite(pen.y) ||
			!std::isfinite(pen.extent.x) || !std::isfinite(pen.extent.y) ||
			pen.extent.x <= 0.0f || pen.extent.y <= 0.0f)
		{
			throw std::runtime_error(where + " must retain finite positive measured text bounds for every rounded percentage.");
		}
		return pen.extent;
	};
	const auto check_stick_region = [&](const char* role, const Vector2F& size,
		const Vector2F& position, float top, float bottom)
	{
		const std::string key = std::string("menu_text.stick_readouts.") + role +
			((std::string_view(role) == "connected" || std::string_view(role) == "disconnected") ?
				" with menu_text.labels.pad" : "");
		const RectangleF bounds(position, size);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			!std::isfinite(top) || !std::isfinite(bottom) || !help_area.contains(bounds) ||
			bounds.y < top || bounds.bottom() > bottom)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside its controls preview/readout region without overlapping the adjacent preview or help text.");
		}
		return bounds;
	};
	const Vector2F raw_caption_position = preview.raw_movement.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y);
	const Vector2F response_caption_position = preview.movement_response.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y);
	const MenuText& stick_text = this->menu_text_;
	for (const bool connected : { true, false })
	{
		const char* role = connected ? "connected" : "disconnected";
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			const Vector2F size = label_size(std::string("menu_text.stick_readouts.") + role + " with menu_text.labels.pad",
				stick_text.stick_connection(slot, connected), controls.help_font);
			check_stick_region(role, size, controls.controls_position, help_area.y, raw_caption_position.y);
		}
	}
	check_stick_region("raw_movement", stick_numeric_size("raw_movement", stick_text.stick_readouts.raw_movement),
		raw_caption_position, help_area.y, preview.raw_movement.y);
	check_stick_region("movement_response", stick_numeric_size("movement_response", stick_text.stick_readouts.movement_response),
		response_caption_position, preview.raw_movement.bottom(), preview.movement_response.y);
	for (const auto& readout : { std::pair("aim_inactive", &stick_text.stick_readouts.aim_inactive),
		std::pair("aim_active", &stick_text.stick_readouts.aim_active) })
	{
		check_stick_region(readout.first, stick_numeric_size(readout.first, *readout.second),
			preview.aim_readout_position, preview.aim.y, preview.association_position.y);
		if (preview.aim_readout_position.x < preview.aim.right())
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.stick_readouts." + readout.first +
				" must remain beside the aim preview.");
		}
	}

	// The remaining label consumers have different fonts, shadows and badge
	// geometry. Validate their complete finite outputs, retaining their existing
	// unfitted layouts and the Pad row's existing OptionRow fit.
	const auto check_label_bounds = [&](const std::string& key, const RectangleF& bounds, const RectangleF& inside)
	{
		if (!finite_stage_rectangle(bounds) || !inside.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its paper including its actual text, shadow, badge and row bounds.");
		}
	};
	const auto shadow_bounds = [](const Vector2F& position, const Vector2F& size, const Vector2F& offset)
	{
		return RectangleF::union_of(RectangleF(position, size), RectangleF(position + offset, size));
	};
	using LabelRows = std::array<std::array<RectangleF, input_slots::count>, 4>;
	LabelRows team_rows;
	LabelRows weapon_rows;
	LabelRows results_rows;
	const SpriteSheet* team_sheet = render.sprite_sheet(main.team_select_sheet);
	for (int player = 0; player < 4; ++player)
	{
		const float row = static_cast<float>(player + 2);
		const Vector2F team_position = this->menu_.layout.position + Vector2F(0.0f, main.team_select.spacing.y * row);
		const Vector2F weapon_position = this->menu_.layout.position + Vector2F(0.0f, main.weapon_select.spacing.y * row);
		RectangleF team_icons;
		float first_team_x = std::numeric_limits<float>::infinity();
		for (size_t icon = 0; icon < main.team_select_frames.size(); ++icon)
		{
			const Vector2F position = team_position + Vector2F(main.team_select.spacing.x * 2.0f +
				main.team_select.size.x * static_cast<float>(icon), 0.0f);
			const RectangleF canvas(position, main.team_select.size);
			const SpriteFrame& frame = team_sheet->sprite_frame(team_sheet->resolve_sprite_frame(main.team_select_frames[icon]));
			const RectangleF bounds = sprite_world_bounds(canvas, frame.source_rectangle(), 0.0f, frame.origin());
			check_label_bounds("menu_text.labels.roster_row at Team Select icons", bounds, team_inside);
			team_icons = icon == 0 ? bounds : RectangleF::union_of(team_icons, bounds);
			if (icon == 0)
			{
				const SpriteFrame& selected = team_sheet->sprite_frame(team_sheet->resolve_sprite_frame(main.team_select_selected_frames[icon]));
				const RectangleF selected_bounds = sprite_world_bounds(canvas, selected.source_rectangle(), 0.0f, selected.origin());
				check_label_bounds("menu_text.labels.roster_row at Team Select selected icon", selected_bounds, team_inside);
				first_team_x = std::min(bounds.x, selected_bounds.x);
			}
		}
		const Vector2F weapon_canvas_position = weapon_position + Vector2F(main.weapon_select.spacing.x * 2.0f, 0.0f);
		RectangleF weapon_tile(weapon_canvas_position, main.weapon_select.size);
		weapon_tile.inflate(this->menu_.icon_padding);
		weapon_tile.inflate(this->menu_.rule_height);
		RectangleF weapon_contents = weapon_tile;
		for (size_t choice = 0; choice < artwork.choices.size(); ++choice)
		{
			const Vector2F name_position = weapon_canvas_position + Vector2F(0.0f,
				main.weapon_select.size.y + this->menu_.icon_padding.y + this->menu_.rule_height);
			const Vector2F name_size = render.measure_text(name_font, artwork.choices[choice].name);
			weapon_contents = RectangleF::union_of(weapon_contents,
				shadow_bounds(name_position, name_size, this->menu_.detail_shadow_offset));
			const Vector2F description_position = weapon_canvas_position + Vector2F(main.weapon_description_x_offset, 0.0f);
			const std::wstring wrapped = text_wrap::wrap(render, name_font, main.weapon_descriptions[choice], main.weapon_description_max_width);
			const Vector2F description_size = render.measure_text(name_font, wrapped);
			weapon_contents = RectangleF::union_of(weapon_contents,
				shadow_bounds(description_position, description_size, main.weapon_description_shadow_offset));
		}
		for (int slot = 0; slot < input_slots::count; ++slot)
		{
			const std::wstring words = this->menu_text_.roster_label(player, slot);
			const std::string key = label_key(slot);
			const RectangleF team_label = shadow_bounds(team_position,
				label_size(key + " at Team Select", words, this->menu_.detail_font), this->menu_.item_shadow_offset);
			RectangleF team_card = RectangleF::union_of(team_label, UiPlayerBadge::before(team_label, this->player_identity_));
			team_card = RectangleF::union_of(team_card, team_icons);
			team_card.inflate(main.setup_focus_padding);
			check_label_bounds(key + " at Team Select", team_card, team_inside);
			if (!std::isfinite(first_team_x) || team_label.right() > first_team_x)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					" at Team Select must end before the first choice icon in either captured frame.");
			}
			team_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = team_card;
			// Weapon passes no offset to UiTextDropShadow: its transparent
			// default (2,2) still contributes to bounds.
			const RectangleF weapon_label = shadow_bounds(weapon_position,
				label_size(key + " at Weapon Select", words, this->menu_.detail_font), Vector2F(2.0f, 2.0f));
			RectangleF weapon_row = RectangleF::union_of(weapon_label, UiPlayerBadge::before(weapon_label, this->player_identity_));
			weapon_row = RectangleF::union_of(weapon_row, weapon_contents);
			check_label_bounds(key + " at Weapon Select", weapon_row, weapon_inside);
			if (weapon_label.right() > weapon_tile.x)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					" at Weapon Select must end before the first weapon tile including its border.");
			}
			weapon_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = weapon_row;
		}
	}
	const auto check_roster_rows = [&](const LabelRows& rows, float heading_bottom, float bottom,
		const char* site)
	{
		// Every depth is a legal 1-4 seat roster; slots are unique. Checking
		// only real assignments avoids inventing two keyboard/mouse seats.
		const auto visit = [&](const auto& self, int player, unsigned int used, float previous_bottom) -> void
		{
			for (int slot = 0; slot < input_slots::count; ++slot)
			{
				const unsigned int bit = 1u << slot;
				if ((used & bit) != 0u) { continue; }
				const RectangleF& bounds = rows[static_cast<size_t>(player)][static_cast<size_t>(slot)];
				if (!std::isfinite(heading_bottom) || !std::isfinite(bottom) ||
					bounds.y < previous_bottom || bounds.bottom() > bottom)
				{
					throw std::runtime_error(this->source_path_ + ": " + label_key(slot) + " at " + site +
						" must follow the heading and complete preceding rows without overlap, before the following content.");
				}
				if (player < 3) { self(self, player + 1, used | bit, bounds.bottom()); }
			}
		};
		visit(visit, 0, 0u, heading_bottom);
	};
	check_roster_rows(team_rows, team_heading.bottom(), team_warning.y, "Team Select");
	check_roster_rows(weapon_rows, weapon_heading.bottom(), weapon_inside.bottom(), "Weapon Select");

	// A missing list contains distinct pad slots in roster order; keyboard
	// never goes missing. Skipping a seat or choosing one remaining pad gives
	// all 208 nonempty reachable lists, with the existing two-column joins.
	const std::string missing_key = "menu_text.labels.roster_row with menu_text.labels.pad at Pause";
	const auto missing_lists = [&](const auto& self, int player, unsigned int used,
		int count, const std::wstring& words) -> void
	{
		if (player == 4)
		{
			if (count == 0) { return; }
			const Vector2F size = label_size(missing_key, words, this->menu_.detail_font);
			const RectangleF bounds((1920.0f - size.x) / 2.0f, pause.status_y, size.x, size.y);
			check_label_bounds(missing_key, bounds, confirmation_paper);
			if (!std::isfinite(pause.layout.position.y) || !std::isfinite(pause.art.focus_padding.y) ||
				bounds.bottom() > pause.layout.position.y - pause.art.focus_padding.y)
			{
				throw std::runtime_error(this->source_path_ + ": " + missing_key +
					" must fit before the first Pause action including its focus padding.");
			}
			return;
		}
		self(self, player + 1, used, count, words);
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			const unsigned int bit = 1u << slot;
			if ((used & bit) != 0u) { continue; }
			std::wstring next = words;
			if (count > 0) { next += count % 2 == 0 ? L"\n" : L"   "; }
			next += this->menu_text_.roster_label(player, slot);
			self(self, player + 1, used | bit, count + 1, next);
		}
	};
	missing_lists(missing_lists, 0, 0u, 0, std::wstring());

	const std::string results_key = "menu_text.labels.roster_row at Results";
	const Font& contribution_font = *render.font(render.resolve_sprite_font(results.contribution_font));
	std::ignore = label_size(results_key, L"0123456789.%", results.contribution_font);
	struct PercentageSuffix
	{
		std::wstring words;
		std::array<const Glyph*, 6> glyphs = {};
	};
	std::array<PercentageSuffix, 1001> percentages;
	for (size_t tenth = 0; tenth < percentages.size(); ++tenth)
	{
		PercentageSuffix& suffix = percentages[tenth];
		// These are the legal one-decimal outputs, not rounded float samples:
		// LevelEndInfo's formatter truncates std::to_wstring at one decimal.
		suffix.words = std::to_wstring(tenth / 10) + L"." + std::to_wstring(tenth % 10) + L"%";
		for (size_t character = 0; character < suffix.words.size(); ++character)
		{
			suffix.glyphs[character] = &contribution_font.drawn(static_cast<char32_t>(suffix.words[character]));
		}
	}
	for (int player = 0; player < 4; ++player)
	{
		const Vector2F position = results.contribution_position + Vector2F(0.0f,
			results.contribution_spacing * static_cast<float>(player + 1));
		for (int slot = 0; slot < input_slots::count; ++slot)
		{
			const std::string key = label_key(slot) + " at Results";
			const std::string where = this->source_path_ + ": " + key;
			RectangleF envelope;
			bool first = true;
			for (const wchar_t* team : { L" | Team A | ", L" | Team B | " })
			{
				const std::wstring prefix = this->menu_text_.roster_label(player, slot) + team;
				std::ignore = label_size(key, prefix, results.contribution_font);
				LabelPen prefix_pen;
				append_label_text(prefix_pen, contribution_font, prefix, where);
				for (const PercentageSuffix& suffix : percentages)
				{
					// Walk each fixed prefix once, then append each cached suffix in
					// Font's sequential FLOAT order. Regrouped sums would change
					// rounding for some finite bearings at an inclusive fit edge.
					LabelPen pen = prefix_pen;
					for (size_t character = 0; character < suffix.words.size(); ++character)
					{
						append_label_glyph(pen, *suffix.glyphs[character], suffix.words[character],
							contribution_font.line_spacing(), where);
					}
					// Results explicitly uses zero shadow. Include each actual badge
					// position: shorter lines can put its top above a taller line's.
					const RectangleF text(position, pen.extent);
					const RectangleF row = RectangleF::union_of(text, UiPlayerBadge::before(text, this->player_identity_));
					check_label_bounds(key, row, results_panel);
					envelope = first ? row : RectangleF::union_of(envelope, row);
					first = false;
				}
			}
			results_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = envelope;
		}
	}
	check_roster_rows(results_rows, contribution_heading.bottom(), results.proceed_y, "Results");
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		const std::string key = "menu_text.labels.pad at Pad Sticks value";
		const Vector2F raw = label_size(key, this->menu_text_.device_label(slot), controls.item_font);
		if (!std::isfinite(controls_value_limit.x) || !std::isfinite(controls_value_limit.y) ||
			controls_value_limit.x <= 0.0f || controls_value_limit.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key + " requires finite positive value limits.");
		}
		float scale = 1.0f;
		if (raw.x > controls_value_limit.x) { scale = controls_value_limit.x / raw.x; }
		if (raw.y * scale > controls_value_limit.y) { scale = controls_value_limit.y / raw.y; }
		const Vector2F fitted = raw * scale;
		const RectangleF bounds(controls.layout.position + Vector2F(controls.layout.spacing.x, 0.0f), fitted);
		if (!std::isfinite(scale) || scale <= 0.0f || !finite_stage_rectangle(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key + " must retain finite positive fitted bounds.");
		}
		check_fixed_paper(key, bounds, controls.box_size, controls.art.focus_padding);
		if (fitted.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y ||
			bounds.right() + controls.art.focus_padding.x > controls.controls_position.x ||
			bounds.bottom() + controls.art.focus_padding.y > controls.status_position.y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its row before the controls help and status including focus padding.");
		}
	}
	for (const int best_of : { 3, 5 })
	{
		check_stage_summary("best_of_mode", this->menu_text_.stage_best_of_mode(best_of), this->menu_.detail_font, 1.0f, 2.0f);
	}
	for (int players = 1; players <= 4; ++players)
	{
		check_stage_summary(players == 1 ? "solo_roster" : "versus_roster", this->menu_text_.stage_roster(players),
			this->menu_.brand_font, 2.0f, 3.0f);
	}
	// Only glyph coverage is independent of the stage catalog. Count-aware
	// geometry is checked after the actual total and eligibility are known.
	std::ignore = fixed_menu_size("menu_text.stage.numbered_navigation",
		this->menu_text_.stage_navigation(1, 1) + L"0123456789", this->menu_.detail_font);
	std::ignore = fixed_menu_size("menu_text.stage.random_hint",
		this->menu_text_.stage_random_hint(1) + L"0123456789", this->menu_.brand_font);
	// Controls volumes and Sticks effective deadzones share the same captured
	// item font and OptionRow value columns at rows one and two. Validate every
	// integer output, including saved non-step volumes and inherited 100%.
	for (int value = 0; value <= 100; ++value)
	{
		const std::wstring text = this->menu_text_.setting_percentage(value);
		for (const int row : { 1, 2 })
		{
			const Vector2F position = controls.layout.position +
				Vector2F(controls.layout.spacing.x, controls.layout.spacing.y * static_cast<float>(row));
			check_value("percentage", text, controls.item_font, position, controls_value_limit,
				controls.box_size, controls.art, controls.layout.spacing.y, true);
		}
	}
	// No tuning value belongs to Presentation alone. Validate each original
	// duration template and its literal glyphs here; the joint content check
	// measures actual defaults/choices, and Mode checks retained values at refresh.
	for (const auto& field : { std::pair("duration_minutes", &values.duration_minutes),
		std::pair("duration_default", &values.duration_default) })
	{
		const std::string where = this->source_path_ + ": menu_text.values." + field.first;
		try
		{
			const std::wstring literal = format_label_template(*field.second, { { L"{value}", L"" } });
			drawable_text::check_font_can_draw(&render, this->menu_.item_font, literal, where);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(where + ": " + error.what());
		}
	}
	// Fullscreen sizes are positive integers, not a closed set of presets.
	// Check every generated glyph and retain the value column's complete fit
	// area: a shorter, narrower value can stay taller than a long fitted one.
	const std::string resolution_key = "menu_text.values.resolution";
	try
	{
		const std::wstring literal = format_label_template(values.resolution,
			{ { L"{width}", L"" }, { L"{height}", L"" } });
		drawable_text::check_font_can_draw(&render, this->menu_.item_font,
			literal + L"0123456789", this->source_path_ + ": " + resolution_key);
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->source_path_ + ": " + resolution_key + ": " + error.what());
	}
	const auto check_resolution = [&](const Vector2I& size)
	{
		check_value("resolution", this->menu_text_.setting_resolution(size.x, size.y), this->menu_.item_font,
			options_value_position(0), options_value_limit, main.options_box_size, options_art,
			main.options_spacing.y, false);
	};
	ResolutionManager preset;
	for (int index = 0; index <= static_cast<int>(ScreenResolution::max); ++index)
	{
		preset.set_resolution(static_cast<ScreenResolution>(index));
		check_resolution(preset.resolution_ivec());
	}
	const int largest = std::numeric_limits<int>::max();
	for (const Vector2I size : { Vector2I(1, 1), Vector2I(1600, 900),
		Vector2I(1, largest), Vector2I(largest, 1), Vector2I(largest, largest) })
	{
		check_resolution(size);
	}
	// OptionRow always resets to scale 1, then fits width and height. Each
	// actual finite positive output therefore stays within this rectangle;
	// INT_MAX is a domain endpoint, not an assertion about the widest digits.
	check_fixed_paper(resolution_key, RectangleF(options_value_position(0), options_value_limit),
		main.options_box_size, options_art.focus_padding);
	// Session selects one of these plain, unscaled labels for End's first
	// action. Retain the same paper, padding and row rule as its other actions.
	const Vector2F end_top = (Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f;
	const Vector2F end_bottom = end_top + end.box_size;
	for (const auto& field : { std::pair("rematch", &this->menu_text_.end.rematch),
		std::pair("play_series_again", &this->menu_text_.end.play_series_again),
		std::pair("replay_draw", &this->menu_text_.end.replay_draw),
		std::pair("next_round", &this->menu_text_.end.next_round) })
	{
		const std::string key = std::string("menu_text.end.") + field.first;
		const Vector2F size = fixed_menu_size(key, *field.second, this->menu_.item_font);
		const Vector2F position = end.layout.position;
		const Vector2F padding = end.art.focus_padding;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x - padding.x < end_top.x || position.y - padding.y < end_top.y ||
			position.x + size.x + padding.x > end_bottom.x ||
			position.y + size.y + padding.y > end_bottom.y ||
			size.y + padding.y * 2.0f > end.layout.spacing.y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its paper and row including focus padding.");
		}
	}
	// Results uses zero-offset transparent text shadows. Center the actual
	// unscaled text bounds on the full-height paper, as its widgets do.
	const auto check_results_centered = [&](const char* role, const std::wstring& text,
		const std::string& font, float y)
	{
		const std::string key = std::string("menu_text.results.") + role;
		const Vector2F size = fixed_menu_size(key, text, font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !results_panel.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Results paper.");
		}
	};
	check_results_centered("single_match_heading", this->menu_text_.results.single_match_heading,
		this->menu_.heading_font, results.heading_y);
	check_results_centered("proceed", this->menu_text_.results.proceed,
		this->menu_.detail_font, results.proceed_y);
}

void Presentation::validate_stage_counts(const RenderResources& render, size_t total, size_t eligible) const
{
	if (total == 0 || eligible > total)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.stage requires a positive stage total and an eligible count no greater than that total.");
	}
	const auto size_of = [&](const std::string& key, const std::wstring& text, const std::string& font)
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
	const auto finite_rectangle = [](const RectangleF& bounds)
	{
		return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
			std::isfinite(bounds.width) && std::isfinite(bounds.height) &&
			std::isfinite(bounds.right()) && std::isfinite(bounds.bottom()) &&
			bounds.width > 0.0f && bounds.height > 0.0f;
	};
	const auto overlaps = [](const RectangleF& first, const RectangleF& second)
	{
		return first.x < second.right() && first.right() > second.x &&
			first.y < second.bottom() && first.bottom() > second.y;
	};
	const MainMenuPresentation& main = this->main_menu_;
	const StageSelectPresentation& stage = main.stage_select;
	const Vector2F shadow = this->menu_.detail_shadow_offset;
	const float edge = main.art.keyline_width * 3.0f;
	const float inset = edge + main.setup_shadow_offset;
	const RectangleF outer((Vector2F(1920.0f, this->menu_.footer_y) - main.stage_box_size) / 2.0f,
		main.stage_box_size);
	const RectangleF inside(outer.position() + Vector2F(edge),
		outer.size() - Vector2F(edge * 2.0f + main.setup_shadow_offset));
	const RectangleF briefing(stage.briefing.position, stage.briefing.size);
	RectangleF preview_edge(stage.preview_position, stage.preview_size);
	preview_edge.inflate(this->menu_.rule_height);
	const RectangleF design(0.0f, 0.0f, 1920.0f, 1080.0f);
	const std::string navigation_key = "menu_text.stage.numbered_navigation";
	// Check every actual index: proportional digits and carried bearings can
	// make an earlier number wider than the final one. The zero-based offset
	// keeps the size_t domain from overflowing after its terminal index.
	for (size_t offset = 0; offset < total; ++offset)
	{
		const size_t index = offset + 1;
		std::wstring text;
		try { text = this->menu_text_.stage_navigation(index, total); }
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(this->source_path_ + ": " + navigation_key + ": " + error.what());
		}
		const Vector2F size = size_of(navigation_key, text, this->menu_.detail_font);
		const RectangleF bounds = RectangleF::union_of(RectangleF(stage.navigation_position, size),
			RectangleF(stage.navigation_position + shadow, size));
		if (!std::isfinite(shadow.x) || !std::isfinite(shadow.y) || !std::isfinite(this->menu_.footer_y) ||
			!finite_rectangle(bounds) || !finite_rectangle(inside) || !finite_rectangle(preview_edge) ||
			!finite_rectangle(briefing) || !inside.contains(bounds) || !design.contains(bounds) ||
			bounds.bottom() > this->menu_.footer_y || overlaps(bounds, preview_edge) || overlaps(bounds, briefing))
		{
			throw std::runtime_error(this->source_path_ + ": " + navigation_key +
				" must fit unscaled inside the Stage paper and design area before the footer without overlapping the preview edge or briefing card, including its shadow.");
		}
		const float record_room = stage.preview_position.x + stage.preview_size.x - bounds.right() -
			this->menu_.focus_padding.x;
		if (!std::isfinite(record_room) || record_room <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + navigation_key +
				" must leave finite positive room for the stage record after menu_style focus padding.");
		}
	}
	if (eligible == 0) { return; }
	const std::string hint_key = "menu_text.stage.random_hint";
	std::wstring hint;
	try { hint = this->menu_text_.stage_random_hint(eligible); }
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->source_path_ + ": " + hint_key + ": " + error.what());
	}
	const Vector2F size = size_of(hint_key, hint, this->menu_.brand_font);
	const RectangleF raw = RectangleF::union_of(RectangleF(Vector2F::ZERO, size), RectangleF(shadow, size));
	const float width = stage.briefing.size.x - 2.0f * stage.briefing_padding.x;
	if (!std::isfinite(width) || width <= 0.0f || !finite_rectangle(raw))
	{
		throw std::runtime_error(this->source_path_ + ": " + hint_key +
			" must have a finite positive hint width and text bounds including its shadow.");
	}
	// Match the page's existing width-only fit, including the signed shadow.
	const float scale = std::min(1.0f, width / raw.width);
	const Vector2F position = stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 7.0f;
	const Vector2F fitted = size * scale;
	const RectangleF bounds = RectangleF::union_of(RectangleF(position, fitted),
		RectangleF(position + shadow * scale, fitted));
	RectangleF padded = bounds;
	padded.inflate(inset);
	const RectangleF card = RectangleF::union_of(briefing, padded);
	if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(edge) || !std::isfinite(inset) ||
		!finite_rectangle(bounds) || !finite_rectangle(card) || !briefing.contains(bounds) ||
		!design.contains(bounds) || bounds.bottom() > this->menu_.footer_y ||
		!inside.contains(card) || overlaps(card, preview_edge))
	{
		throw std::runtime_error(this->source_path_ + ": " + hint_key +
			" must fit its briefing panel, expanded Stage paper and design area before the footer using width-only fitting, including its scaled shadow.");
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
