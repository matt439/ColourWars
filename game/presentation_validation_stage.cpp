#include "game/pch.h"

// Presentation validation for Stage Select. The phase functions in
// presentation_validation.cpp call these in diagnostic order; the count-aware
// checks run later, from validate_stage_counts (presentation_validation_catalog.cpp).
#include "game/presentation_validation.h"
#include "game/drawable_text.h"
#include "game/menu_geometry.h"

#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace mattmath;
using menu_consts::DESIGN_RESOLUTION;

// Phase 2: the Random name, the briefing's status and summary rows and the
// stage records. Leaves the briefing panel and its rows in the geometry.
void Presentation::Validation::validate_stage_select_briefing(StageGeometry& geometry) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const MenuValueText& values = this->presentation_.menu_text_.values;
	const std::string random_name_key = "menu_text.values.random";
	const Vector2F random_name_size = fixed_menu_size(random_name_key, values.random, this->presentation_.menu_.item_font);
	const StageSelectPresentation& stage = main.stage_select;
	const Vector2F name_shadow = this->presentation_.menu_.item_shadow_offset;
	const float name_bottom = stage.preview_position.y - this->presentation_.menu_.rule_height;
	if (!std::isfinite(name_shadow.x) || !std::isfinite(name_shadow.y) ||
		!std::isfinite(stage.preview_size.x) || stage.preview_size.x <= 0.0f ||
		!std::isfinite(name_bottom) || name_bottom <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + random_name_key +
			" must have a finite shadow, positive preview width and positive name area.");
	}
	const RectangleF random_name_union = RectangleF::union_of(RectangleF(Vector2F::ZERO, random_name_size),
		RectangleF(name_shadow, random_name_size));
	if (!std::isfinite(random_name_union.x) || !std::isfinite(random_name_union.y) ||
		!std::isfinite(random_name_union.width) || !std::isfinite(random_name_union.height) ||
		random_name_union.width <= 0.0f || random_name_union.height <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + random_name_key +
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
		!RectangleF(0.0f, 0.0f, DESIGN_RESOLUTION.x, name_bottom).contains(fitted_random_name))
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + random_name_key +
			" must fit its stage-name design area above the preview edge including its scaled shadow.");
	}
	const RectangleF stage_briefing(stage.briefing.position, stage.briefing.size);
	const Vector2F stage_info = stage.briefing.position + stage.briefing_padding;
	const Vector2F stage_status_position = stage_info + stage.briefing.spacing * 6.0f;
	const float stage_hint_top = (stage_info + stage.briefing.spacing * 7.0f).y;
	const auto check_stage_status = [&](const char* role, const std::wstring& text) -> RectangleF
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->presentation_.menu_.detail_font);
		const Vector2F shadow = this->presentation_.menu_.detail_shadow_offset;
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
			!menu_geometry::design_area().contains(bounds) || bounds.bottom() > stage_hint_top)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the briefing panel and design area before the authored hint row origin, including its shadow.");
		}
		return bounds;
	};
	// The page grows its briefing card around every row it draws; the
	// expanded card is checked after the summaries' (validate_generated_values).
	std::vector<std::pair<const char*, RectangleF>> briefing_rows;
	briefing_rows.emplace_back("browse_status", check_stage_status("browse_status", this->presentation_.menu_text_.stage.browse_status));
	briefing_rows.emplace_back("locked_status", check_stage_status("locked_status", this->presentation_.menu_text_.stage.locked_status));
	briefing_rows.emplace_back("briefing_label", check_stage_briefing(stage_briefing, stage_info, "briefing_label", this->presentation_.menu_text_.stage.briefing_label, this->presentation_.menu_.brand_font, 0.0f, 1.0f));
	briefing_rows.emplace_back("goal", check_stage_briefing(stage_briefing, stage_info, "goal", this->presentation_.menu_text_.stage.goal, this->presentation_.menu_.brand_font, 3.0f, 4.0f));
	briefing_rows.emplace_back("solo_objective", check_stage_briefing(stage_briefing, stage_info, "solo_objective", this->presentation_.menu_text_.stage.solo_objective, this->presentation_.menu_.brand_font, 4.0f, 6.0f));
	briefing_rows.emplace_back("versus_objective", check_stage_briefing(stage_briefing, stage_info, "versus_objective", this->presentation_.menu_text_.stage.versus_objective, this->presentation_.menu_.brand_font, 4.0f, 6.0f));
	briefing_rows.emplace_back("solo_mode", check_stage_briefing(stage_briefing, stage_info, "solo_mode", this->presentation_.menu_text_.stage.solo_mode, this->presentation_.menu_.detail_font, 1.0f, 2.0f));
	briefing_rows.emplace_back("single_match_mode", check_stage_briefing(stage_briefing, stage_info, "single_match_mode", this->presentation_.menu_text_.stage.single_match_mode, this->presentation_.menu_.detail_font, 1.0f, 2.0f));
	// Stage records share the navigation row, right-aligned to the preview,
	// and the page shrinks a line to the room beside the arena counter. The
	// copy must draw in its font and the row must sit below the preview edge
	// and above the footer.
	const RectangleF record_preview(stage.preview_position, stage.preview_size);
	for (const std::wstring* label : { &this->presentation_.menu_text_.records.best_win, &this->presentation_.menu_text_.records.best_coverage })
	{
		const std::wstring line = *label + L" 100.0% / 99 " + this->presentation_.menu_text_.records.played;
		const Vector2F size = fixed_menu_size("menu_text.records", line, stage.record_font) +
			this->presentation_.menu_.detail_shadow_offset;
		if (stage.record_y < record_preview.bottom() + this->presentation_.menu_.rule_height ||
			stage.record_y + size.y > this->presentation_.menu_.footer_y)
		{
			throw std::runtime_error(this->presentation_.source_path_ +
				": main_menu.stage_select.record_y and record_font must place menu_text.records "
				"below the preview edge and above the footer.");
		}
	}
	geometry.stage_briefing = stage_briefing;
	geometry.stage_info = stage_info;
	geometry.briefing_rows = std::move(briefing_rows);
}

// Phase 2: the Random counter and the hint rows, which join the briefing rows.
void Presentation::Validation::validate_stage_select_navigation(StageGeometry& geometry) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const StageSelectPresentation& stage = main.stage_select;
	const RectangleF& stage_briefing = geometry.stage_briefing;
	const Vector2F& stage_info = geometry.stage_info;
	std::vector<std::pair<const char*, RectangleF>>& briefing_rows = geometry.briefing_rows;
	const auto stage_rectangles_overlap = [](const RectangleF& first, const RectangleF& second)
	{
		return first.x < second.right() && first.right() > second.x &&
			first.y < second.bottom() && first.bottom() > second.y;
	};
	const std::string navigation_key = "menu_text.stage.random_navigation";
	const Vector2F navigation_size = fixed_menu_size(navigation_key, this->presentation_.menu_text_.stage.random_navigation,
		this->presentation_.menu_.detail_font);
	const Vector2F stage_detail_shadow = this->presentation_.menu_.detail_shadow_offset;
	const RectangleF navigation_bounds = RectangleF::union_of(RectangleF(stage.navigation_position, navigation_size),
		RectangleF(stage.navigation_position + stage_detail_shadow, navigation_size));
	RectangleF preview_edge(stage.preview_position, stage.preview_size);
	preview_edge.inflate(this->presentation_.menu_.rule_height);
	if (!std::isfinite(stage_detail_shadow.x) || !std::isfinite(stage_detail_shadow.y) ||
		!std::isfinite(this->presentation_.menu_.footer_y) || !finite_stage_rectangle(navigation_bounds) ||
		!finite_stage_rectangle(preview_edge) || !finite_stage_rectangle(stage_briefing) ||
		!menu_geometry::design_area().contains(navigation_bounds) ||
		navigation_bounds.bottom() > this->presentation_.menu_.footer_y ||
		stage_rectangles_overlap(navigation_bounds, preview_edge) ||
		stage_rectangles_overlap(navigation_bounds, stage_briefing))
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + navigation_key +
			" must fit unscaled inside the design area before the footer without overlapping the preview edge or briefing panel, including its shadow.");
	}
	const Vector2F stage_hint_position = stage_info + stage.briefing.spacing * 7.0f;
	const float stage_hint_width = stage.briefing.size.x - 2.0f * stage.briefing_padding.x;
	const auto check_stage_hint = [&](const char* role, const std::wstring& text) -> RectangleF
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->presentation_.menu_.brand_font);
		const RectangleF raw = RectangleF::union_of(RectangleF(Vector2F::ZERO, size),
			RectangleF(stage_detail_shadow, size));
		if (!std::isfinite(stage_hint_width) || stage_hint_width <= 0.0f || !finite_stage_rectangle(raw))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must have a finite positive hint width and text bounds including its shadow.");
		}
		// Match refresh: measure at scale one and fit only the full union's width.
		const float scale = std::min(1.0f, stage_hint_width / raw.width);
		const Vector2F fitted_size = size * scale;
		const RectangleF bounds = RectangleF::union_of(RectangleF(stage_hint_position, fitted_size),
			RectangleF(stage_hint_position + stage_detail_shadow * scale, fitted_size));
		if (!std::isfinite(scale) || scale <= 0.0f || !finite_stage_rectangle(bounds) ||
			!stage_briefing.contains(bounds) || !menu_geometry::design_area().contains(bounds) ||
			bounds.bottom() > this->presentation_.menu_.footer_y)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit its briefing panel and design area before the footer using width-only fitting, including its scaled shadow.");
		}
		return bounds;
	};
	briefing_rows.emplace_back("locked_hint", check_stage_hint("locked_hint", this->presentation_.menu_text_.stage.locked_hint));
	briefing_rows.emplace_back("browse_hint", check_stage_hint("browse_hint", this->presentation_.menu_text_.stage.browse_hint));
	briefing_rows.emplace_back("user_hint", check_stage_hint("user_hint", this->presentation_.menu_text_.stage.user_hint));
	geometry.random_navigation = navigation_bounds;
}

// Phase 5: Stage Select's heading and the loser's-pick headings.
void Presentation::Validation::validate_stage_heading() const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const RectangleF stage_inside = menu_geometry::paper_interior(
		menu_geometry::footer_paper(main.stage_box_size, this->presentation_.menu_.footer_y),
		menu_geometry::setup_art(main));
	const std::wstring& stage_heading_words = this->presentation_.menu_text_.heading(MenuHeading::arena);
	// Loser's pick (ROAD#20) puts the picking team's heading in its place.
	const auto check_stage_heading = [&](const std::string& key, const std::wstring& words)
	{
		drawable_text::check_font_can_draw(&render, this->presentation_.menu_.heading_font, words, this->presentation_.source_path_ + ": " + key);
		const Vector2F stage_heading_size = render.measure_text(render.resolve_sprite_font(this->presentation_.menu_.heading_font),
			words);
		const RectangleF stage_heading = RectangleF::union_of(
			RectangleF(this->presentation_.menu_.layout.position, stage_heading_size),
			RectangleF(this->presentation_.menu_.layout.position + this->presentation_.menu_.heading_shadow_offset, stage_heading_size));
		if (!std::isfinite(stage_heading.x) || !std::isfinite(stage_heading.y) ||
			!std::isfinite(stage_heading.width) || !std::isfinite(stage_heading.height) ||
			stage_heading.width <= 0.0f || stage_heading.height <= 0.0f || !stage_inside.contains(stage_heading))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit main_menu.stage_box_size including its retained shadow bounds.");
		}
	};
	check_stage_heading("menu_text.headings.arena", stage_heading_words);
	check_stage_heading("menu_text.stage.team_a_picks", this->presentation_.menu_text_.stage.team_a_picks);
	check_stage_heading("menu_text.stage.team_b_picks", this->presentation_.menu_text_.stage.team_b_picks);
}

// Phase 8: the briefing summaries, the card every row grows and the Random
// counter on the Stage paper.
void Presentation::Validation::validate_stage_summaries(const StageGeometry& stage_geometry) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const auto& stage_briefing = stage_geometry.stage_briefing;
	const auto& stage_info = stage_geometry.stage_info;
	// The page draws one card around every row it shows at once, so the rows
	// are also checked together, each alternative included, for the preview.
	RectangleF shown_together = stage_geometry.briefing_rows.front().second;
	for (const int best_of : { 3, 5 })
	{
		shown_together = RectangleF::union_of(shown_together, check_stage_summary(stage_briefing, stage_info, "best_of_mode", this->presentation_.menu_text_.stage_best_of_mode(best_of), this->presentation_.menu_.detail_font, 1.0f, 2.0f));
	}
	for (int players = 1; players <= 4; ++players)
	{
		shown_together = RectangleF::union_of(shown_together, check_stage_summary(stage_briefing, stage_info, players == 1 ? "solo_roster" : "versus_roster", this->presentation_.menu_text_.stage_roster(players),
			this->presentation_.menu_.brand_font, 2.0f, 3.0f));
	}
	// The summaries' rules for the other rows. The numbered counter's paper
	// rule holds for the static Random one; Random hides the stage record, so
	// it needs no record room.
	const RectangleF stage_inside = menu_geometry::paper_interior(
		menu_geometry::footer_paper(main.stage_box_size, this->presentation_.menu_.footer_y),
		menu_geometry::setup_art(main));
	if (!finite_stage_rectangle(stage_inside) || !stage_inside.contains(stage_geometry.random_navigation))
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.stage.random_navigation must fit unscaled inside the Stage paper, including its shadow.");
	}
	for (const auto& [role, bounds] : stage_geometry.briefing_rows)
	{
		check_briefing_card(stage_briefing, std::string("menu_text.stage.") + role, bounds);
		shown_together = RectangleF::union_of(shown_together, bounds);
	}
	// Each row fits the Stage paper alone, so only the preview can refuse the
	// whole card: a row widening it and another deepening it can together
	// reach a preview sitting diagonally from the briefing.
	check_briefing_card(stage_briefing, "menu_text.stage briefing rows shown together", shown_together);
	// Only glyph coverage is independent of the stage catalog. Count-aware
	// geometry is checked after the actual total and eligibility are known.
	std::ignore = fixed_menu_size("menu_text.stage.numbered_navigation",
		this->presentation_.menu_text_.stage_navigation(1, 1) + L"0123456789", this->presentation_.menu_.detail_font);
	std::ignore = fixed_menu_size("menu_text.stage.random_hint",
		this->presentation_.menu_text_.stage_random_hint(1) + L"0123456789", this->presentation_.menu_.brand_font);
}
