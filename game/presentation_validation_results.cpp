#include "game/pch.h"

// Presentation validation for the Results page. The phase functions in
// presentation_validation.cpp call these in diagnostic order.
#include "game/presentation_validation.h"
#include "game/drawable_text.h"
#include "game/menu_geometry.h"
#include "game/states/ui_player_badge.h"

#include <array>
#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>

using namespace labrador;
using namespace mattmath;
using namespace presentation_read;
using namespace presentation_validation;

// Phase 2: the Results panel and its record lines. Leaves the panel, the score
// bar's edges and the contribution heading in the geometry.
void Presentation::Validation::validate_results_records(StageGeometry& geometry) const
{
	const ResultsMenuPresentation& results = this->presentation_.results_menu_;
	const RectangleF results_panel = menu_geometry::screen_paper(results.box_size);
	const RectangleF results_fill = menu_geometry::results_fill_box(results);
	const float results_fill_right = results_fill.right();
	const float results_fill_bottom = results_fill.bottom();
	const RectangleF contribution_heading(results.contribution_position, fixed_menu_size(
		"menu_text.results.contribution_heading", this->presentation_.menu_text_.results.contribution_heading,
		results.contribution_font));
	for (const std::wstring* label : { &this->presentation_.menu_text_.records.new_best, &this->presentation_.menu_text_.records.stage_best })
	{
		const Vector2F size = fixed_menu_size("menu_text.records", *label + L" 100.0%", results.contribution_font);
		const RectangleF bounds(Vector2F(results_fill_right - size.x, results.record_y), size);
		if (!results_panel.contains(bounds) || bounds.y < results_fill_bottom ||
			bounds.bottom() > results.proceed_y || bounds.x <= contribution_heading.right())
		{
			throw std::runtime_error(this->presentation_.source_path_ +
				": results_menu.record_y and menu_text.records must fit the panel below the score bar, "
				"above the prompt and to the right of the contribution heading.");
		}
	}
	geometry.results_panel = results_panel;
	geometry.results_fill_right = results_fill_right;
	geometry.results_fill_bottom = results_fill_bottom;
	geometry.contribution_heading = contribution_heading;
}

// Phase 7: every contribution row and award line.
void Presentation::Validation::validate_results_contributions(const StageGeometry& stage_geometry) const
{
	const ResultsMenuPresentation& results = this->presentation_.results_menu_;
	const auto& results_panel = stage_geometry.results_panel;
	const auto& results_fill_right = stage_geometry.results_fill_right;
	const auto& contribution_heading = stage_geometry.contribution_heading;
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
	LabelRows results_rows;
	for (int player = 0; player < 4; ++player)
	{
		const Vector2F position = results.contribution_position + Vector2F(0.0f,
			results.contribution_spacing * static_cast<float>(player + 1));
		for (int slot = 0; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
		{
			const std::string key = label_key(slot) + " at Results";
			RectangleF envelope;
			bool first = true;
			for (const std::wstring* team : { &this->presentation_.menu_text_.results.contribution_team_a,
				&this->presentation_.menu_text_.results.contribution_team_b })
			{
				const std::string team_key = key + " with menu_text.results." +
					(team == &this->presentation_.menu_text_.results.contribution_team_a ? "contribution_team_a" : "contribution_team_b");
				const std::string where = this->presentation_.source_path_ + ": " + team_key;
				const std::wstring prefix = label_words(player, slot) + *team;
				std::ignore = label_size(team_key, prefix, results.contribution_font);
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
					const RectangleF row = RectangleF::union_of(text, UiPlayerBadge::before(text, this->presentation_.player_identity_));
					check_label_bounds(team_key, row, results_panel);
					envelope = first ? row : RectangleF::union_of(envelope, row);
					first = false;
				}
			}
			results_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = envelope;
		}
	}
	check_roster_rows(results_rows, contribution_heading.bottom(), results.proceed_y,
		"Results with menu_text.results.contribution_team_a and menu_text.results.contribution_team_b");
	// An award sits at the right of a contribution row, right-aligned to the
	// score bar. The page leaves out one that would meet its own row's text,
	// so the rows above stay the only bound on how long a label may be.
	for (const auto& [award, key] : { std::pair{ MatchAward::paint_laid, "award_paint_laid" },
		std::pair{ MatchAward::recaptured, "award_recaptured" },
		std::pair{ MatchAward::airtime, "award_airtime" },
		std::pair{ MatchAward::splats, "award_splats" } })
	{
		const std::string award_key = std::string("menu_text.results.") + key;
		const Vector2F size = fixed_menu_size(award_key,
			this->presentation_.menu_text_.results.award_line(award), results.contribution_font);
		for (int player = 0; player < 4; ++player)
		{
			const RectangleF bounds(Vector2F(results_fill_right - size.x,
				results.contribution_position.y +
					results.contribution_spacing * static_cast<float>(player + 1)), size);
			if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
				!results_panel.contains(bounds) || bounds.bottom() > results.proceed_y)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + award_key +
					" must fit the Results panel at the end of every contribution row, above the prompt.");
			}
		}
	}
}

// Phase 9: Results' headings, Session lines and percentages.
void Presentation::Validation::validate_results_lines(const StageGeometry& stage_geometry) const
{
	const ResultsMenuPresentation& results = this->presentation_.results_menu_;
	const auto& results_panel = stage_geometry.results_panel;
	// Results uses zero-offset transparent text shadows. Center the actual
	// unscaled text bounds on the full-height paper, as its widgets do.
	const auto check_results_centered = [&](const char* role, const std::wstring& text,
		const std::string& font, float y)
	{
		const std::string key = std::string("menu_text.results.") + role;
		const Vector2F size = fixed_menu_size(key, text, font);
		const RectangleF bounds(menu_geometry::centred_x(size.x), y, size.x, size.y);
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
	const RectangleF round_bounds(menu_geometry::centred_x(round_size.x), results.heading_y, round_size.x, round_size.y);
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
	const Vector2F results_fill_position = menu_geometry::results_fill_box(results).position();
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
	}
}
