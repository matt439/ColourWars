#pragma once

// Private implementation of the ordered resource-validation walk. Construction
// only borrows services and creates the original ResourceValidation wrapper;
// page geometry and font lookups stay at their original points in the walk.
#include "game/presentation_read.h"
#include "game/resource_validation.h"
#include "game/presentation_validation_metrics.h"
#include "game/input_slots.h"
#include "game/player_control.h"
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct Presentation::Validation
{
    using Vector2F = mattmath::Vector2F;
    using RectangleF = mattmath::RectangleF;
    using FontHandle = labrador::FontHandle;
    static constexpr std::array<BotDifficulty, 3> label_difficulties{
        BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
    static constexpr int label_choices = input_slots::count + static_cast<int>(label_difficulties.size());
    using LabelRows = std::array<std::array<RectangleF, label_choices>, 4>;

    struct SetupGeometry
    {
        MenuPaperArtPresentation options_art, display_art;
        RectangleF setup_inside, confirmation_paper;
    };
    struct StageGeometry
    {
        RectangleF stage_briefing;
        Vector2F stage_info;
        RectangleF results_panel;
        float results_fill_right, results_fill_bottom;
        RectangleF contribution_heading;
        // Each briefing row and the Random counter, for the checks that run
        // after the summaries (the expanded card and the Stage paper).
        std::vector<std::pair<const char*, RectangleF>> briefing_rows;
        RectangleF random_navigation;
    };
    struct ControlsGeometry
    {
        Vector2F options_value_limit, controls_value_limit;
        RectangleF help_area;
        float probe_height;
    };
    struct RosterPapers
    {
        RectangleF team_inside, team_heading, team_warning, weapon_inside, weapon_heading;
    };
    // The Back cards that four pages' status lines keep clear of.
    struct StatusBacks
    {
        RectangleF options, controls, pad_buttons, sticks;
    };
    // Sticks' help text and aim caption, which its status lines keep clear of.
    struct SticksHelp
    {
        RectangleF pad_association, aim_cutoff;
        float probe_height;
    };
    struct StatusMessage { const char* key; const std::wstring& text; };

    const Presentation& presentation_;
    const labrador::RenderResources& render;
    const ResourceValidation check;

    Validation(const Presentation& presentation, const labrador::RenderResources& render_resources,
        const labrador::AudioResources& audio) : presentation_(presentation), render(render_resources),
        check(presentation.source_path_, render_resources, audio) {}
    void validate();

	SetupGeometry validate_menu_actions();
	StageGeometry validate_stage_text();
	ControlsGeometry validate_controls_text(const SetupGeometry& setup);
	FontHandle validate_artwork_resources();
	RosterPapers validate_additional_papers(const SetupGeometry& setup);
	void validate_roster_labels(const SetupGeometry& setup, const ControlsGeometry& control_geometry,
	const RosterPapers& papers, FontHandle name_font);
	void validate_contributions(const StageGeometry& stage_geometry, const ControlsGeometry& control_geometry);
	void validate_generated_values(const SetupGeometry& setup, const StageGeometry& stage_geometry,
	const ControlsGeometry& control_geometry);
	void validate_results_text(const StageGeometry& stage_geometry);

	// The phases' page steps, one file per page; only the phase functions above
	// call them, and only in presentation_validation.cpp, which owns the order.
	// presentation_validation_menu.cpp: chrome, Home, Credits and Title.
	void validate_menu_chrome() const;
	void validate_home_actions() const;
	void validate_credits_text() const;
	void validate_title_and_home_text() const;
	StatusBacks status_back_cards(const SetupGeometry& setup) const;
	// presentation_validation_options.cpp
	void validate_options_actions(const SetupGeometry& setup) const;
	Vector2F validate_options_values(const SetupGeometry& setup) const;
	void validate_options_statuses(const SetupGeometry& setup, const RectangleF& options_back) const;
	void validate_options_palette_timing(const SetupGeometry& setup) const;
	void validate_options_resolutions(const SetupGeometry& setup, const ControlsGeometry& control_geometry) const;
	// presentation_validation_pause.cpp: Pause, practice Pause, Exit and Display.
	void validate_pause_actions() const;
	void validate_exit_actions() const;
	void validate_display_actions(const SetupGeometry& setup) const;
	void validate_confirmations(const SetupGeometry& setup) const;
	void validate_exit_heading(const SetupGeometry& setup) const;
	void validate_display_text(const SetupGeometry& setup) const;
	void validate_pause_connection(const SetupGeometry& setup) const;
	void validate_pause_missing_lists(const SetupGeometry& setup) const;
	// presentation_validation_end.cpp
	void validate_end_actions() const;
	void validate_end_headings() const;
	void validate_end_session_labels() const;
	// presentation_validation_settings.cpp: Controls, Sticks, Keyboard Jump,
	// Pad Buttons and Profiles.
	void validate_controls_actions() const;
	void validate_sticks_actions() const;
	Vector2F validate_settings_toggles() const;
	RectangleF validate_controls_help(const RectangleF& help_area) const;
	void validate_keyboard_jump_text(const RectangleF& help_area) const;
	RectangleF validate_pad_buttons_text(const RectangleF& help_area) const;
	RectangleF validate_profiles_text(const RectangleF& help_area) const;
	SticksHelp validate_sticks_help(const RectangleF& help_area) const;
	void validate_controls_statuses(const RectangleF& controls_back, const RectangleF& controls_help) const;
	void validate_keyboard_jump_statuses(const RectangleF& help_area) const;
	void validate_pad_buttons_statuses(const RectangleF& pad_buttons_back, const RectangleF& pad_buttons_help) const;
	void validate_profiles_statuses(const RectangleF& profiles_help) const;
	void validate_sticks_statuses(const RectangleF& sticks_back, const SticksHelp& sticks_help) const;
	void validate_stick_readouts(const ControlsGeometry& control_geometry) const;
	void validate_sticks_pad_values(const ControlsGeometry& control_geometry) const;
	void validate_settings_percentages(const ControlsGeometry& control_geometry) const;
	void validate_sticks_shared_template() const;
	// presentation_validation_setup.cpp: Mode, Player Count, Join, Team Select
	// and Weapon Select.
	void validate_setup_actions(const SetupGeometry& setup) const;
	void validate_team_heading(RosterPapers& papers) const;
	void validate_weapon_heading(RosterPapers& papers) const;
	void validate_bot_labels() const;
	void validate_join_labels(const SetupGeometry& setup) const;
	void validate_team_and_weapon_labels(const RosterPapers& papers, FontHandle name_font) const;
	void validate_mode_duration_templates() const;
	// presentation_validation_stage.cpp
	void validate_stage_select_briefing(StageGeometry& geometry) const;
	void validate_stage_select_navigation(StageGeometry& geometry) const;
	void validate_stage_heading() const;
	void validate_stage_summaries(const StageGeometry& stage_geometry) const;
	// presentation_validation_results.cpp
	void validate_results_records(StageGeometry& geometry) const;
	void validate_results_contributions(const StageGeometry& stage_geometry) const;
	void validate_results_lines(const StageGeometry& stage_geometry) const;

	// Checks more than one page's step shares: Back cards and status lines
	// (menu), and the settings pages' action rows and help blocks (settings).
	RectangleF padded_back(const Vector2F& position, const std::string& font,
		const MenuPaperArtPresentation& art) const;
	void check_status(const StatusMessage& message, const std::string& font,
		const Vector2F& position, const Vector2F& paper_size, const MenuPaperArtPresentation& art,
		std::initializer_list<RectangleF> obstacles) const;
	void check_controls_item(MenuItem item, int row, bool fitted_label) const;
	RectangleF check_help_fit(const std::string& key, const std::wstring& text,
		const Vector2F& position, const Vector2F& available, const RectangleF& help_area) const;

	void check_menu_line(const std::string& key, const std::wstring& text,
		const std::string& font, const Vector2F& position, float bottom, const Vector2F& shadow) const;
	Vector2F check_menu_item(MenuItem item, const std::string& font, const Vector2F& position,
		const RectangleF& paper, float row_spacing, const std::optional<float>& label_width,
		const MenuPaperArtPresentation& art, bool uniform_fit = false) const;
	RectangleF check_setup_paper(const RectangleF& setup_inside, const std::string& key, const std::wstring& words,
		const std::string& font, const Vector2F& position, const Vector2F& shadow,
		bool action, float row_spacing) const;
	Vector2F fixed_menu_size(const std::string& key, const std::wstring& text, const std::string& font) const;
	void check_fixed_paper(const std::string& key, RectangleF bounds,
		const Vector2F& paper_size, const Vector2F& padding) const;
	void check_value(const char* role, const std::wstring& text, const std::string& font,
		const Vector2F& position, const Vector2F& limit, const Vector2F& paper_size,
		const MenuPaperArtPresentation& art, float row_spacing, bool controls_value) const;
	RectangleF check_stage_briefing(const RectangleF& stage_briefing, const Vector2F& stage_info, const char* role, const std::wstring& text, const std::string& font,
		float line, float next_line) const;
	RectangleF check_stage_summary(const RectangleF& stage_briefing, const Vector2F& stage_info, const char* role, const std::wstring& text, const std::string& font,
		float line, float next_line) const;
	void check_briefing_card(const RectangleF& stage_briefing, const std::string& key, RectangleF bounds) const;
	static bool finite_stage_rectangle(const RectangleF& bounds);
	Vector2F options_value_position(int row) const;
	std::wstring label_words(int player, int choice) const;
	static std::string label_key(int slot);
	Vector2F label_size(const std::string& key, const std::wstring& words, const std::string& font_name) const;
	void check_label_bounds(const std::string& key, const RectangleF& bounds, const RectangleF& inside) const;
	static RectangleF shadow_bounds(const Vector2F& position, const Vector2F& size, const Vector2F& offset);
	void check_roster_rows(const LabelRows& rows, float heading_bottom, float bottom,
		const char* site) const;

};
