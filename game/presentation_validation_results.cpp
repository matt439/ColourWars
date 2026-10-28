#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/player_profiles.h"
#include "game/objects/player_badge.h"
#include "game/resource_validation.h"
#include "game/controls_text.h"
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
using namespace presentation_read;
using namespace presentation_validation;

void Presentation::Validation::validate_results_text(const StageGeometry& stage_geometry)
{
	const EndMenuPresentation& end = this->presentation_.end_menu_;
	const ResultsMenuPresentation& results = this->presentation_.results_menu_;
	const auto& results_panel = stage_geometry.results_panel;
	// Session selects one of these plain, unscaled labels for End's first
	// action. Retain the same paper, padding and row rule as its other actions.
	const Vector2F end_top = (Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f;
	const Vector2F end_bottom = end_top + end.box_size;
	for (const auto& field : { std::pair("rematch", &this->presentation_.menu_text_.end.rematch),
		std::pair("play_series_again", &this->presentation_.menu_text_.end.play_series_again),
		std::pair("replay_draw", &this->presentation_.menu_text_.end.replay_draw),
		std::pair("next_round", &this->presentation_.menu_text_.end.next_round) })
	{
		const std::string key = std::string("menu_text.end.") + field.first;
		const Vector2F size = fixed_menu_size(key, *field.second, this->presentation_.menu_.item_font);
		const Vector2F position = end.layout.position;
		const Vector2F padding = end.art.focus_padding;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x - padding.x < end_top.x || position.y - padding.y < end_top.y ||
			position.x + size.x + padding.x > end_bottom.x ||
			position.y + size.y + padding.y > end_bottom.y ||
			size.y + padding.y * 2.0f > end.layout.spacing.y)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
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
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the Results paper.");
		}
	};
	check_results_centered("single_match_heading", this->presentation_.menu_text_.results.single_match_heading,
		this->presentation_.menu_.heading_font, results.heading_y);
	check_results_centered("proceed", this->presentation_.menu_text_.results.proceed,
		this->presentation_.menu_.detail_font, results.proceed_y);
	check_results_centered("series_team_a_wins", this->presentation_.menu_text_.results.series_team_a_wins,
		this->presentation_.menu_.detail_font, results.session_result_y);
	check_results_centered("series_team_b_wins", this->presentation_.menu_text_.results.series_team_b_wins,
		this->presentation_.menu_.detail_font, results.session_result_y);
	check_results_centered("draw_no_win", this->presentation_.menu_text_.results.draw_no_win,
		this->presentation_.menu_.detail_font, results.session_result_y);
	check_results_centered("series_continues", this->presentation_.menu_text_.results.series_continues,
		this->presentation_.menu_.detail_font, results.session_result_y);
	// Measure every reachable ledger, including single-match Sessions.
	// Session completion prevents both teams reaching the target.
	try
	{
		for (int target = 1; target <= 3; ++target)
		{
			for (int team_a_wins = 0; team_a_wins <= target; ++team_a_wins)
			{
				for (int team_b_wins = 0; team_b_wins <= target; ++team_b_wins)
				{
					if (team_a_wins == target && team_b_wins == target) { continue; }
					check_results_centered("series_score",
						this->presentation_.menu_text_.results.score_line(team_a_wins, team_b_wins, target),
						this->presentation_.menu_.detail_font, results.session_score_y);
				}
			}
		}
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.results.series_score: " + error.what());
	}
	const std::string round_key = "menu_text.results.round_heading";
	std::wstring round_literals;
	try
	{
		round_literals = format_label_template(this->presentation_.menu_text_.results.round_heading, { { L"{round}", L"" } });
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + round_key + ": " + error.what());
	}
	const std::string round_where = this->presentation_.source_path_ + ": " + round_key;
	drawable_text::check_font_can_draw(&render, this->presentation_.menu_.heading_font,
		round_literals + L"0123456789", round_where);
	const Font& round_font = *render.font(render.resolve_sprite_font(this->presentation_.menu_.heading_font));
	const Vector2F round_size = round_heading_size(round_font, this->presentation_.menu_text_.results.round_heading, round_where);
	const RectangleF round_bounds((1920.0f - round_size.x) / 2.0f, results.heading_y, round_size.x, round_size.y);
	if (!std::isfinite(round_bounds.x) || !std::isfinite(round_bounds.y) || !results_panel.contains(round_bounds))
	{
		throw std::runtime_error(round_where + " must fit unscaled inside the Results paper for every positive int round.");
	}
	check_results_centered("competitive_team_a_wins", this->presentation_.menu_text_.results.competitive_team_a_wins,
		this->presentation_.menu_.item_font, results.winner_y);
	check_results_centered("competitive_team_b_wins", this->presentation_.menu_text_.results.competitive_team_b_wins,
		this->presentation_.menu_.item_font, results.winner_y);
	check_results_centered("competitive_draw", this->presentation_.menu_text_.results.competitive_draw,
		this->presentation_.menu_.item_font, results.winner_y);
	// These are reference rendering outputs, not a restriction on the public
	// float arithmetic or percentage_text's spellings. Wrappers accept the
	// caller's already-rendered value without parsing or sanitizing it.
	for (const auto& field : { std::pair("solo_coverage", &this->presentation_.menu_text_.results.solo_coverage),
		std::pair("team_a_percentage", &this->presentation_.menu_text_.results.team_a_percentage),
		std::pair("team_b_percentage", &this->presentation_.menu_text_.results.team_b_percentage) })
	{
		try
		{
			std::ignore = format_label_template(*field.second, { { L"{percentage}", L"" } });
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.results." + field.first + ": " + error.what());
		}
	}
	const Vector2F results_fill_position((1920.0f - results.fill_box_size.x) / 2.0f,
		results_panel.y + results.fill_box_y_offset);
	const Vector2F percentage_a_position = results_fill_position + results.team_fill_border;
	const Vector2F percentage_b_top_right = results_fill_position + Vector2F(results.fill_box_size.x, 0.0f) +
		Vector2F(-results.team_fill_border.x, results.team_fill_border.y);
	const auto check_results_percentage = [&](const char* role, const std::wstring& text, bool right_aligned)
	{
		const std::string key = std::string("menu_text.results.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->presentation_.menu_.detail_font);
		const Vector2F position = right_aligned ?
			Vector2F(percentage_b_top_right.x - size.x, percentage_b_top_right.y) : percentage_a_position;
		RectangleF plate(position, size);
		// MenuPaperArtwork draws its face and shade within the supplied bounds.
		// Match the widgets' single padding inflation and zero text shadow.
		plate.inflate(results.art.focus_padding);
		if (!finite_stage_rectangle(plate) || !results_panel.contains(plate))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the Results paper including its percentage plate padding.");
		}
	};
	for (int tenth = 0; tenth <= 1000; ++tenth)
	{
		const std::wstring percentage = std::to_wstring(tenth / 10) + L"." + std::to_wstring(tenth % 10) + L"%";
		check_results_centered("solo_coverage", this->presentation_.menu_text_.results.solo_coverage_line(percentage),
			this->presentation_.menu_.item_font, results.winner_y);
		const std::wstring team_a = this->presentation_.menu_text_.results.team_a_percentage_line(percentage);
		const std::wstring team_b = this->presentation_.menu_text_.results.team_b_percentage_line(percentage);
		check_results_percentage("team_a_percentage", team_a, false);
		check_results_percentage("team_b_percentage", team_b, true);
		// The monitor has its own font. Check the complete reference strings;
		// RevealArt keeps its existing runtime width/height fit and outline.
		std::ignore = fixed_menu_size("menu_text.results.team_a_percentage at monitor", team_a,
			this->presentation_.opening_.percent_font);
		std::ignore = fixed_menu_size("menu_text.results.team_b_percentage at monitor", team_b,
			this->presentation_.opening_.percent_font);
	}}
