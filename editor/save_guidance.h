#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace colourwars_editor
{
    struct ContentCatalog;

    // Feedback only: does not write, register, change the destination or opt a
    // stage into Random. `my_stages` is absent when the user's folder is unknown.
    std::wstring save_guidance(const std::filesystem::path& destination,
        const std::filesystem::path& source_levels, const ContentCatalog& catalog,
        const std::optional<std::filesystem::path>& my_stages, bool saved);

    // Empty unless `destination` is a registered source level (in
    // `source_levels`, named by the manifest) whose `display_name` differs from
    // its presentation.json menu_text.stage_names entry, or has none: the game
    // then refuses to start, so the editor says which two must agree. Never
    // blocks a save; registration (stage_registration.h) adds the entry.
    std::wstring stage_name_warning(const std::filesystem::path& destination,
        const std::filesystem::path& source_levels, const ContentCatalog& catalog,
        const std::string& display_name);
}
