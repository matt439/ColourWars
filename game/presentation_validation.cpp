#include "game/pch.h"
#include "game/presentation_validation.h"

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
    LabelRows results_rows = this->validate_roster_labels(setup, controls, papers, name_font);
    this->validate_contributions(stage, controls, results_rows);
    this->validate_generated_values(setup, stage, controls);
    this->validate_results_text(stage);
}
