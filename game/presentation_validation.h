#pragma once

// Private implementation of the ordered resource-validation walk. Construction
// only borrows services and creates the original ResourceValidation wrapper;
// page geometry and font lookups stay at their original points in the walk.
#include "game/presentation_read.h"
#include "game/resource_validation.h"
#include "game/presentation_validation_metrics.h"
#include "game/input_slots.h"
#include "game/player_control.h"
#include <optional>

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
	LabelRows validate_roster_labels(const SetupGeometry& setup, const ControlsGeometry& control_geometry,
	const RosterPapers& papers, FontHandle name_font);
	void validate_contributions(const StageGeometry& stage_geometry, const ControlsGeometry& control_geometry,
	LabelRows& results_rows);
	void validate_generated_values(const SetupGeometry& setup, const StageGeometry& stage_geometry,
	const ControlsGeometry& control_geometry);
	void validate_results_text(const StageGeometry& stage_geometry);


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
	void check_stage_summary(const RectangleF& stage_briefing, const Vector2F& stage_info, const char* role, const std::wstring& text, const std::string& font,
		float line, float next_line) const;
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
