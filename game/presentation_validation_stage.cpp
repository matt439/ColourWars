#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/menu_geometry.h"
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
using menu_consts::DESIGN_RESOLUTION;

Presentation::Validation::StageGeometry Presentation::Validation::validate_stage_text()
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
	const auto check_stage_status = [&](const char* role, const std::wstring& text)
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
	};
	check_stage_status("browse_status", this->presentation_.menu_text_.stage.browse_status);
	check_stage_status("locked_status", this->presentation_.menu_text_.stage.locked_status);
	check_stage_briefing(stage_briefing, stage_info, "briefing_label", this->presentation_.menu_text_.stage.briefing_label, this->presentation_.menu_.brand_font, 0.0f, 1.0f);
	check_stage_briefing(stage_briefing, stage_info, "goal", this->presentation_.menu_text_.stage.goal, this->presentation_.menu_.brand_font, 3.0f, 4.0f);
	check_stage_briefing(stage_briefing, stage_info, "solo_objective", this->presentation_.menu_text_.stage.solo_objective, this->presentation_.menu_.brand_font, 4.0f, 6.0f);
	check_stage_briefing(stage_briefing, stage_info, "versus_objective", this->presentation_.menu_text_.stage.versus_objective, this->presentation_.menu_.brand_font, 4.0f, 6.0f);
	check_stage_briefing(stage_briefing, stage_info, "solo_mode", this->presentation_.menu_text_.stage.solo_mode, this->presentation_.menu_.detail_font, 1.0f, 2.0f);
	check_stage_briefing(stage_briefing, stage_info, "single_match_mode", this->presentation_.menu_text_.stage.single_match_mode, this->presentation_.menu_.detail_font, 1.0f, 2.0f);
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
	const auto check_stage_hint = [&](const char* role, const std::wstring& text)
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
	};
	check_stage_hint("locked_hint", this->presentation_.menu_text_.stage.locked_hint);
	check_stage_hint("browse_hint", this->presentation_.menu_text_.stage.browse_hint);
	check_stage_hint("user_hint", this->presentation_.menu_text_.stage.user_hint);

	return { stage_briefing, stage_info, results_panel, results_fill_right, results_fill_bottom, contribution_heading };
}

void Presentation::Validation::validate_generated_values(const SetupGeometry& setup, const StageGeometry& stage_geometry,
	const ControlsGeometry& control_geometry)
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const MenuValueText& values = this->presentation_.menu_text_.values;
	const auto& stage_briefing = stage_geometry.stage_briefing;
	const auto& stage_info = stage_geometry.stage_info;
	const auto& controls_value_limit = control_geometry.controls_value_limit;
	const auto& options_value_limit = control_geometry.options_value_limit;
	const auto& options_art = setup.options_art;
	for (const int best_of : { 3, 5 })
	{
		check_stage_summary(stage_briefing, stage_info, "best_of_mode", this->presentation_.menu_text_.stage_best_of_mode(best_of), this->presentation_.menu_.detail_font, 1.0f, 2.0f);
	}
	for (int players = 1; players <= 4; ++players)
	{
		check_stage_summary(stage_briefing, stage_info, players == 1 ? "solo_roster" : "versus_roster", this->presentation_.menu_text_.stage_roster(players),
			this->presentation_.menu_.brand_font, 2.0f, 3.0f);
	}
	// Only glyph coverage is independent of the stage catalog. Count-aware
	// geometry is checked after the actual total and eligibility are known.
	std::ignore = fixed_menu_size("menu_text.stage.numbered_navigation",
		this->presentation_.menu_text_.stage_navigation(1, 1) + L"0123456789", this->presentation_.menu_.detail_font);
	std::ignore = fixed_menu_size("menu_text.stage.random_hint",
		this->presentation_.menu_text_.stage_random_hint(1) + L"0123456789", this->presentation_.menu_.brand_font);
	// Controls volumes occupy rows two and three; Sticks effective deadzones
	// occupy rows one and two in the same captured font/value column. Validate every
	// integer output, including saved non-step volumes and inherited 100%.
	for (int value = 0; value <= 100; ++value)
	{
		const std::wstring text = this->presentation_.menu_text_.setting_percentage(value);
		for (const int row : { 1, 2, 3, 4 })
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
		std::pair("duration_default", &values.duration_default), std::pair("shared", &values.shared) })
	{
		const std::string where = this->presentation_.source_path_ + ": menu_text.values." + field.first;
		try
		{
			const std::wstring literal = format_label_template(*field.second, { { L"{value}", L"" } });
			drawable_text::check_font_can_draw(&render, this->presentation_.menu_.item_font, literal, where);
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
		drawable_text::check_font_can_draw(&render, this->presentation_.menu_.item_font,
			literal + L"0123456789", this->presentation_.source_path_ + ": " + resolution_key);
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + resolution_key + ": " + error.what());
	}
	const auto check_resolution = [&](const Vector2I& size)
	{
		check_value("resolution", this->presentation_.menu_text_.setting_resolution(size.x, size.y), this->presentation_.menu_.item_font,
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
}
