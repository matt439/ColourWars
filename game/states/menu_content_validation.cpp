#include "game/pch.h"
#include "game/states/menu_content_validation.h"
#include "game/content_validation.h"
#include "game/drawable_text.h"
#include "game/series_stage_rule.h"
#include "game/states/main_menu.h"

using namespace mattmath;
using namespace labrador;

void validate_mode_stage_rules(RenderResources* render, const Presentation& presentation)
{
	const MenuStyle& style = presentation.menu();
	const MainMenuPresentation& look = presentation.main_menu();
	const MenuText& text = presentation.menu_text();
	const RectangleF panel = menu_geometry::footer_paper(look.setup_box_size, style.footer_y);
	const MenuPaperArtPresentation art = menu_geometry::setup_art(look);
	for (const SeriesStageRule rule : { SeriesStageRule::same, SeriesStageRule::rotate, SeriesStageRule::losers_pick })
	{
		const std::wstring& value = text.series_stage_value(rule);
		drawable_text::check_font_can_draw(render, style.item_font, value,
			std::string("presentation.json: menu_text.values.series_stage_") + series_stage_rule_key(rule));
		auto row = MainMenuModeSelect::make_stage_rule_row(render, style, look, text);
		row->set_value(value);
		MainMenuModeSelect::check_stage_rule_row(*row, panel, art, look.mode_select_spacing.y);
	}
}

void validate_mode_durations(RenderResources* render,
	const GameTuning& tuning, const Presentation& presentation)
{
	const MenuStyle& style = presentation.menu();
	const MainMenuPresentation& look = presentation.main_menu();
	const MenuText& text = presentation.menu_text();
	const RectangleF panel = menu_geometry::footer_paper(look.setup_box_size, style.footer_y);
	const MenuPaperArtPresentation art = menu_geometry::setup_art(look);
	const auto check = [&](float seconds, bool use_default)
	{
		const std::string where = std::string("presentation.json: menu_text.values.") +
			(use_default ? "duration_default" : "duration_minutes");
		const std::wstring value = text.setting_duration(seconds, use_default);
		drawable_text::check_font_can_draw(render, style.item_font, value, where);
		auto row = MainMenuModeSelect::make_duration_row(render, style, look, text);
		row->set_value(value);
		MainMenuModeSelect::check_duration_row(*row, panel, art, look.mode_select_spacing.y);
	};
	check(tuning.match().duration, true);
	for (const float seconds : tuning.match().duration_choices) { check(seconds, false); }
}

void validate_shell_content(const GameTuning& tuning, const Presentation& presentation,
	RenderResources* render, const AudioResources* audio,
	const Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
	const PlaySettings& play_settings)
{
	validate_game_content(tuning, presentation, render, audio, level_infos, stages, play_settings);
	// Keep these after all established diagnostics, and retain the exact Mode
	// row/shadow/card/shade/arrow geometry without fitting the value.
	validate_mode_durations(render, tuning, presentation);
	validate_mode_stage_rules(render, presentation);
}
