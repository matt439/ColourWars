// Level editor document acceptance. The checks live in level_editor/ and run
// here in a fixed order: they share one scratch directory.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "scripts/acceptance/support/probe.h"

#include <exception>
#include <filesystem>
#include <iostream>

using namespace level_editor_acceptance;

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("level editor acceptance", [&]
    {
        require(argc == 3, "Usage: colourwars_editor_acceptance <repository> <empty-scratch-directory>");
        const fs::path repo = fs::absolute(argv[1]);
        const fs::path scratch = fs::absolute(argv[2]);
        require(fs::is_directory(scratch), "Acceptance scratch directory must already exist");
        check_empty_property_keys(scratch);
        check_alignment(repo, scratch);
        check_shipped_levels(repo, scratch);
        check_format_versions(scratch);
        check_ending_audio(repo);
        check_resource_names(repo, scratch);
        check_blockout_aids(repo, scratch);
        check_unknown_fields_and_rectangles(scratch);
        check_palette_and_ramps(scratch);
        check_random_setting(scratch);
        check_materials(scratch);
        check_drift(repo, scratch);
        check_motion(repo, scratch);
        check_paint_drive(repo, scratch);
        check_team_gate(repo, scratch);
        check_stations(repo, scratch);
        check_station_editing(repo, scratch);
        check_mirror_copy(repo, scratch);
        check_clipboard(repo, scratch);
        check_symmetry_report(repo, scratch);
        check_fixed_slabs(repo, scratch);
        check_history_and_failures(scratch);
        check_external_save_changes(scratch);
        check_save_guidance(scratch);
        check_paint_mechanism_guides(repo, scratch);
        check_editor_window_layout(scratch);
        check_stage_registration(repo, scratch);
        check_user_stage_save(repo, scratch);
        std::cout << "PASS level editor document acceptance\n";
        return 0;
    });
}
