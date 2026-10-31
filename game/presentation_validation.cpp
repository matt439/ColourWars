#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/menu_geometry.h"

using namespace mattmath;

void Presentation::validate_resources(const labrador::RenderResources& render,
    const labrador::AudioResources& audio) const
{
    Validation validation(*this, render, audio);
    validation.validate();
    this->validate_stage_scan_resources(render);
}

void Presentation::Validation::validate()
{
    // This sequence is diagnostic precedence, including deliberately interleaved
    // Stage/Results and menu/resource checks. Contexts retain earlier outputs;
    // their constructors do not resolve or measure anything ahead of this walk.
    const SetupGeometry setup = this->validate_menu_actions();
    const StageGeometry stage = this->validate_stage_text();
    const ControlsGeometry controls = this->validate_controls_text(setup);
    const FontHandle name_font = this->validate_artwork_resources();
    const RosterPapers papers = this->validate_additional_papers(setup);
    this->validate_roster_labels(setup, controls, papers, name_font);
    this->validate_contributions(stage, controls);
    this->validate_generated_values(setup, stage, controls);
    this->validate_results_text(stage);
}

// Each phase below is a list of page steps, each in its page's file. A page
// spans several phases, so the order of these calls, not the files, is the
// order diagnostics are reported in. validate_artwork_resources (phase 4,
// presentation_validation_assets.cpp) checks no page and stays one walk.

Presentation::Validation::SetupGeometry Presentation::Validation::validate_menu_actions()
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	// Pure arithmetic on the records: deriving it first resolves and measures
	// nothing, so no diagnostic moves.
	const SetupGeometry setup{ menu_geometry::setup_art(main), menu_geometry::display_art(pause, main),
		menu_geometry::paper_interior(
			menu_geometry::footer_paper(main.setup_box_size, this->presentation_.menu_.footer_y),
			menu_geometry::setup_art(main)),
		menu_geometry::screen_paper(pause.box_size) };
	this->validate_menu_chrome();
	this->validate_home_actions();
	this->validate_options_actions(setup);
	this->validate_credits_text();
	this->validate_pause_actions();
	this->validate_exit_actions();
	this->validate_display_actions(setup);
	this->validate_end_actions();
	this->validate_controls_actions();
	this->validate_sticks_actions();
	this->validate_setup_actions(setup);
	this->validate_confirmations(setup);
	this->validate_end_headings();
	return setup;
}

Presentation::Validation::StageGeometry Presentation::Validation::validate_stage_text()
{
	// Stage Select and Results interleave: each step fills the fields named
	// in its comment, and the later Stage Select step reads the earlier one's.
	StageGeometry geometry{};
	this->validate_stage_select_briefing(geometry);
	this->validate_results_records(geometry);
	this->validate_stage_select_navigation(geometry);
	return geometry;
}

Presentation::Validation::ControlsGeometry Presentation::Validation::validate_controls_text(const SetupGeometry& setup)
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const RectangleF help_area(controls.controls_position, controls.controls_size);
	const Vector2F options_value_limit = this->validate_options_values(setup);
	const Vector2F controls_value_limit = this->validate_settings_toggles();
	this->validate_title_and_home_text();
	const RectangleF controls_help = this->validate_controls_help(help_area);
	this->validate_keyboard_jump_text(help_area);
	const RectangleF pad_buttons_help = this->validate_pad_buttons_text(help_area);
	const RectangleF profiles_help = this->validate_profiles_text(help_area);
	const SticksHelp sticks_help = this->validate_sticks_help(help_area);
	const StatusBacks backs = this->status_back_cards(setup);
	this->validate_options_statuses(setup, backs.options);
	this->validate_controls_statuses(backs.controls, controls_help);
	this->validate_keyboard_jump_statuses(help_area);
	this->validate_pad_buttons_statuses(backs.pad_buttons, pad_buttons_help);
	this->validate_profiles_statuses(profiles_help);
	this->validate_sticks_statuses(backs.sticks, sticks_help);
	return { options_value_limit, controls_value_limit, help_area, sticks_help.probe_height };
}

Presentation::Validation::RosterPapers Presentation::Validation::validate_additional_papers(const SetupGeometry& setup)
{
	RosterPapers papers{};
	this->validate_team_heading(papers);
	this->validate_weapon_heading(papers);
	this->validate_stage_heading();
	this->validate_exit_heading(setup);
	this->validate_display_text(setup);
	this->validate_options_palette_timing(setup);
	return papers;
}

void Presentation::Validation::validate_roster_labels(const SetupGeometry& setup,
	const ControlsGeometry& control_geometry, const RosterPapers& papers, FontHandle name_font)
{
	this->validate_bot_labels();
	this->validate_join_labels(setup);
	this->validate_pause_connection(setup);
	this->validate_stick_readouts(control_geometry);
	this->validate_team_and_weapon_labels(papers, name_font);
	this->validate_pause_missing_lists(setup);
}

void Presentation::Validation::validate_contributions(const StageGeometry& stage_geometry,
	const ControlsGeometry& control_geometry)
{
	this->validate_results_contributions(stage_geometry);
	this->validate_sticks_pad_values(control_geometry);
}

void Presentation::Validation::validate_generated_values(const SetupGeometry& setup,
	const StageGeometry& stage_geometry, const ControlsGeometry& control_geometry)
{
	this->validate_stage_summaries(stage_geometry);
	this->validate_settings_percentages(control_geometry);
	this->validate_mode_duration_templates();
	this->validate_sticks_shared_template();
	this->validate_options_resolutions(setup, control_geometry);
}

void Presentation::Validation::validate_results_text(const StageGeometry& stage_geometry)
{
	this->validate_end_session_labels();
	this->validate_results_lines(stage_geometry);
}
