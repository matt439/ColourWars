#include "game/pch.h"
#include "game/presentation_menu_text.h"
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

// Reads presentation.json's menu_text section and formats MenuText and
// MenuResultsText from it: label and stick-readout templates.

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;

namespace
{
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
}

namespace presentation_read
{
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
		text.results.series_team_a_wins = read_menu_line(results, "series_team_a_wins");
		text.results.series_team_b_wins = read_menu_line(results, "series_team_b_wins");
		text.results.draw_no_win = read_menu_line(results, "draw_no_win");
		text.results.series_continues = read_menu_line(results, "series_continues");
		text.results.series_score = read_menu_line(results, "series_score");
		try
		{
			std::ignore = text.results.score_line(0, 0, 1);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(results.where() + ": series_score: " + error.what());
		}
		text.results.round_heading = read_menu_line(results, "round_heading");
		try
		{
			std::ignore = text.results.round_title(1);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(results.where() + ": round_heading: " + error.what());
		}
		text.results.competitive_team_a_wins = read_menu_line(results, "competitive_team_a_wins");
		text.results.competitive_team_b_wins = read_menu_line(results, "competitive_team_b_wins");
		text.results.competitive_draw = read_menu_line(results, "competitive_draw");
		text.results.contribution_team_a = read_menu_line(results, "contribution_team_a");
		text.results.contribution_team_b = read_menu_line(results, "contribution_team_b");
		const auto read_results_percentage = [&](const char* key)
		{
			const std::wstring value = read_menu_line(results, key);
			try
			{
				std::ignore = format_label_template(value, { { L"{percentage}", L"0.0%" } });
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(results.where() + ": " + key + ": " + error.what());
			}
			return value;
		};
		text.results.solo_coverage = read_results_percentage("solo_coverage");
		text.results.team_a_percentage = read_results_percentage("team_a_percentage");
		text.results.team_b_percentage = read_results_percentage("team_b_percentage");
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

std::wstring MenuResultsText::solo_coverage_line(const std::wstring& percentage) const
{
	return format_label_template(this->solo_coverage, { { L"{percentage}", percentage } });
}

std::wstring MenuResultsText::team_a_percentage_line(const std::wstring& percentage) const
{
	return format_label_template(this->team_a_percentage, { { L"{percentage}", percentage } });
}

std::wstring MenuResultsText::team_b_percentage_line(const std::wstring& percentage) const
{
	return format_label_template(this->team_b_percentage, { { L"{percentage}", percentage } });
}

std::wstring MenuResultsText::score_line(int team_a_wins, int team_b_wins, int target) const
{
	if (target < 1 || target > 3 || team_a_wins < 0 || team_b_wins < 0 ||
		team_a_wins > target || team_b_wins > target ||
		(team_a_wins == target && team_b_wins == target))
	{
		throw std::invalid_argument("Session score requires a target from 1 to 3, wins from 0 to the target, "
			"and at most one team at the target.");
	}
	return format_label_template(this->series_score,
		{ { L"{team_a_wins}", std::to_wstring(team_a_wins) },
		{ L"{team_b_wins}", std::to_wstring(team_b_wins) },
		{ L"{target}", std::to_wstring(target) } });
}

std::wstring MenuResultsText::round_title(int round) const
{
	if (round <= 0)
	{
		throw std::invalid_argument("Results round heading requires a positive integer round.");
	}
	return format_label_template(this->round_heading, { { L"{round}", std::to_wstring(round) } });
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
