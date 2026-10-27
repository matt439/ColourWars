#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

#include <filesystem>
#include <iostream>

// Shared menu text in presentation.json: defaults, validation, fonts and
// bounds for every menu page. The checks live in menu_text/, grouped by
// page; main runs them in an order that matters (see fixtures.h).
using namespace menu_text_acceptance;

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: menu_text.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        Variants variants(content, std::filesystem::absolute(argv[0]).parent_path() / "menu-text-fixtures");
        World world(content);
        const Presentation defaults((content / "presentation.json").string().c_str());
        check_defaults(defaults);
        defaults.validate_resources(world.render, world.audio);
        check_keyed_content(variants);
        check_text_and_fonts(variants, world, content);
        check_inclusive_bounds(variants, world);
        check_item_bounds(variants, world);
        check_restore_defaults_text(variants, world);
        check_options_palette_timing(variants, world);
        check_setup_text(variants, world);
        check_pause_text(variants, world);
        check_connection_text(variants, world);
        check_connection_bounds(variants, world);
        check_label_text(variants, world, defaults.menu_text());
        check_bot_label_text(variants, world, defaults.menu_text());
        check_label_resources(variants, world);
        check_label_results_extrema(variants, world);
        check_confirmation_text(variants, world);
        check_confirmation_bounds(variants, world);
        check_display_trial_text(variants, world);
        check_end_text(variants, world);
        check_end_headings(variants, world);
        check_end_heading_bounds(variants, world);
        check_controls_text(variants, world);
        check_controls_option_rows(variants, world);
        check_pad_jump_text(variants, world);
        check_setting_percentage_text(variants, world, defaults.menu_text());
        check_setting_percentage_rows(variants, world);
        check_value_text(variants, world);
        check_value_fitting(variants, world);
        check_stage_random_fitting(variants, world);
        check_stage_random_bounds(variants, world);
        check_stage_count_text(variants, world, defaults.menu_text());
        check_stage_count_bounds(variants, world);
        check_stage_summary_text(variants, world, defaults.menu_text());
        check_stage_summary_bounds(variants, world);
        check_stage_briefing_text(variants, world);
        check_stage_briefing_bounds(variants, world);
        check_stage_status_text(variants, world);
        check_stage_status_bounds(variants, world);
        check_stage_cue_text(variants, world);
        check_stage_cue_bounds(variants, world);
        check_title_and_home_text(variants, world, defaults);
        check_title_and_home_bounds(variants, world);
        check_help_text(variants, world);
        check_help_fitting(variants, world);
        check_help_bounds(variants, world);
        check_stick_readout_text(variants, world, defaults.menu_text());
        check_stick_readout_bounds(variants, world);
        check_stick_numeric_extrema(variants, world);
        check_status_text(variants, world);
        check_status_fitting(variants, world);
        check_status_bounds(variants, world);
        check_option_row_fitting(variants, world);
        check_prompt_bar(world, defaults);
        check_setting_duration_text(variants, defaults.menu_text());
        check_setting_duration_content(variants, world, content);
        check_setting_resolution_text(variants, defaults.menu_text());
        check_setting_resolution_resources(variants, world);
        check_end_continuation_text(variants, world);
        check_end_continuation_bounds(variants, world);
        check_results_plain_text(variants, world);
        check_results_plain_bounds(variants, world);
        check_session_result_text(variants, world);
        check_session_result_bounds(variants, world);
        check_series_score_text(variants, defaults.menu_text());
        check_series_score_resources(variants, world, defaults);
        check_round_heading_text(variants, defaults.menu_text());
        check_round_heading_resources(variants, world, defaults);
        check_competitive_winner_text(variants, world);
        check_competitive_winner_selection(defaults.menu_text());
        check_competitive_winner_bounds(variants, world);
        check_contribution_team_text(variants, world);
        check_contribution_team_roster(world, defaults);
        check_contribution_team_bounds(variants, world);
        check_result_percentage_text(variants, defaults.menu_text());
        check_result_percentage_numbers(defaults.menu_text());
        check_result_percentage_resources(variants, world, defaults);
        std::cout << "PASS shared menu text: exact headings/items/complete legends, keyed and enum guards, normalization, "
            "font/paper/row/status/confirmation/End-heading bounds, unscaled centered confirmation/End-heading copy, width-only save-status fitting, "
            "authored OptionRow value fitting, connection schema/player tokens/fresh glyphs and exact Join/Pause/Team bounds, "
            "all101 setting percentages with captured Controls/Sticks value fitting, "
            "authored duration templates, FLOAT/defaultfloat compatibility and actual tuning/default/choice row validation, "
            "authored resolution templates with full positive integer dimensions, strict original tokens and captured Options fitting, "
            "four plain End continuation labels with literal braces, actual-font glyphs and unscaled row0 padding boundaries, "
            "three plain Results roles with zero-offset shadows, centered paper bounds and authored record/roster clearance dependencies, "
            "four plain Session result outcomes with owned zero-shadow text and inclusive unscaled Results-paper bounds, "
            "strict authored series-score tokens, all26 legal ledgers and proportional-font full-paper boundaries, "
            "authored numbered Results headings over the full positive-int domain, "
            "plain competitive winners with original float/solo selection and inclusive actual item-font paper bounds, "
            "authored contribution team separators with sparse-slot credit, sequential full-row font bounds and complete mixed-team badge clearance, "
            "Results percentage wrappers preserving original FLOAT output and arbitrary pre-rendered insertion, "
            "device/roster labels with original tokens, actual fonts, full Pause lists and exact Results percentage bounds, "
            "shadow-inclusive Random arena fitting, unscaled arena briefing/status/navigation, "
            "finite authored series/roster templates and expanded-card boundaries, "
            "actual stage-count templates, all-index proportional bounds and width-fitted Random counts, "
            "Pad Sticks exact-token/range/201-value/schema guards, multiline boundaries and adversarial pen/ink extrema, "
            "width-only arena hints and actual PromptBar measurement compatibility.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL shared menu text: " << error.what() << '\n';
        return 1;
    }
}
