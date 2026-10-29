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
}
