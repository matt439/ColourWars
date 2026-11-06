#pragma once

// What the level_editor acceptance pieces share: the file and JSON helpers,
// the entry and property lookups, the refusal checks, the
// reject-without-change template, and the checks wmain calls. This folder
// is compiled by colourwars_editor_acceptance in editor/CMakeLists.txt, not by
// run-acceptance.ps1's same-named-folder rule; do not add level_editor to that
// script's case list.
#include "editor/level_document.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"
#include <rapidjson/document.h>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace level_editor_acceptance
{
    using namespace colourwars_editor;
    namespace fs = std::filesystem;

    using acceptance::require;
    using acceptance::read_text;
    using acceptance::write_text;
    rapidjson::Document parse(const std::string& text);
    std::string encode(const rapidjson::Document& document);
    bool equal(const Rect& actual, const Rect& expected);
    Entry entry(const LevelDocument& document, EntryKind kind, std::size_t index = 0);
    // The entry of this kind with this label; throws when there is none.
    Entry named(const LevelDocument& document, EntryKind kind, const std::string& name);
    // The inspector row at this path, or nullptr.
    const Property* property(const std::vector<Property>& properties, const std::string& path);
    // The action must throw, and its message must contain expected.
    void refuses(const std::function<void()>& action, const std::string& expected, const char* message);
    // A file the game would refuse at load is refused by the editor at load
    // too, naming the shared rule's own words, and the open document is kept.
    void refuses_file(LevelDocument& document, const fs::path& path, const std::string& expected,
        const char* message);
    // File > New is editor/template.json, which walls in its arena. A fixture
    // that works near the world's edges erases the two walls and the ceiling.
    void erase_template_walls(LevelDocument& document);

    template<typename Action>
    void reject_without_change(LevelDocument& document, Action action, const char* message)
    {
        const std::string before = document.serialize();
        const fs::path path = document.path();
        const bool dirty = document.dirty();
        const bool undo = document.can_undo();
        const bool redo = document.can_redo();
        bool rejected = false;
        try { action(); }
        catch (const std::exception& error)
        {
            rejected = true;
            require(std::string(error.what()).size() > 0, "Failure needs a diagnostic");
        }
        require(rejected, message);
        require(document.serialize() == before && document.path() == path &&
            document.dirty() == dirty && document.can_undo() == undo &&
            document.can_redo() == redo, "Failed operation changed document or history");
    }

    void check_blockout_aids(const fs::path& repo, const fs::path& scratch);
    void check_resource_names(const fs::path& repo, const fs::path& scratch);
    void check_shipped_levels(const fs::path& repo, const fs::path& scratch);
    void check_format_versions(const fs::path& scratch);
    void check_empty_property_keys(const fs::path& scratch);
    void check_alignment(const fs::path& repo, const fs::path& scratch);
    void check_unknown_fields_and_rectangles(const fs::path& scratch);
    void check_ending_audio(const fs::path& repo);
    void check_random_setting(const fs::path& scratch);
    void check_materials(const fs::path& scratch);
    void check_drift(const fs::path& repo, const fs::path& scratch);
    void check_motion(const fs::path& repo, const fs::path& scratch);
    void check_lift_motion(const fs::path& repo, const fs::path& scratch);
    void check_paint_drive(const fs::path& repo, const fs::path& scratch);
    void check_team_gate(const fs::path& repo, const fs::path& scratch);
    void check_stations(const fs::path& repo, const fs::path& scratch);
    void check_station_editing(const fs::path& repo, const fs::path& scratch);
    void check_mirror_copy(const fs::path& repo, const fs::path& scratch);
    void check_clipboard(const fs::path& repo, const fs::path& scratch);
    void check_symmetry_report(const fs::path& repo, const fs::path& scratch);
    void check_fixed_slabs(const fs::path& repo, const fs::path& scratch);
    void check_palette_and_ramps(const fs::path& scratch);
    void check_history_and_failures(const fs::path& scratch);
    void check_external_save_changes(const fs::path& scratch);
    void check_save_guidance(const fs::path& scratch);
    void check_paint_mechanism_guides(const fs::path& repo, const fs::path& scratch);
    void check_editor_window_layout(const fs::path& scratch);
    void check_stage_registration(const fs::path& repo, const fs::path& scratch);
    void check_user_stage_save(const fs::path& repo, const fs::path& scratch);
}
