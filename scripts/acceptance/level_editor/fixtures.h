#pragma once

// What the level_editor acceptance pieces share: the file and JSON helpers,
// the reject-without-change template, and the checks wmain calls. This folder
// is compiled by colourwars_editor_acceptance in editor/CMakeLists.txt, not by
// run-acceptance.ps1's same-named-folder rule; do not add level_editor to that
// script's case list.
#include "editor/level_document.h"
#include <rapidjson/document.h>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <string>

namespace level_editor_acceptance
{
    using namespace colourwars_editor;
    namespace fs = std::filesystem;

    void require(bool condition, const std::string& message);
    std::string contents(const fs::path& path);
    void write(const fs::path& path, const std::string& text);
    rapidjson::Document parse(const std::string& text);
    std::string encode(const rapidjson::Document& document);
    bool equal(const Rect& actual, const Rect& expected);
    Entry entry(const LevelDocument& document, EntryKind kind, std::size_t index = 0);

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
    void check_unknown_fields_and_rectangles(const fs::path& scratch);
    void check_ending_audio(const fs::path& repo);
    void check_random_setting(const fs::path& scratch);
    void check_materials(const fs::path& scratch);
    void check_drift(const fs::path& repo, const fs::path& scratch);
    void check_palette_and_ramps(const fs::path& scratch);
    void check_history_and_failures(const fs::path& scratch);
    void check_stage_registration(const fs::path& repo, const fs::path& scratch);
}
