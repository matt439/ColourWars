#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace colourwars_editor
{
    // A path as UTF-8, for messages and JSON. path.string() converts through
    // the ANSI code page and throws on a name it cannot spell; this does not.
    inline std::string path_text(const std::filesystem::path& path)
    {
        const std::u8string text = path.u8string();
        return std::string(text.begin(), text.end());
    }

    // The inverse: UTF-8 text, such as a JSON string, as a path.
    inline std::filesystem::path utf8_path(std::string_view text)
    {
        return std::filesystem::path(std::u8string(text.begin(), text.end()));
    }
}
