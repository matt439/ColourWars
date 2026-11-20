#pragma once

#include "game/contracts/level_format.h"
#include "scripts/acceptance/support/scratch.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace acceptance
{
    // The game and editor exercise the same ordering around the cutoff.
    // After this setup, y-edge.json would be 256th and zz-last.JSON 257th.
    inline void fill_before_user_stage_limit(const std::filesystem::path& folder, std::string_view stage)
    {
        write_text(folder / "Z-FIRST.json", stage);
        write_text(folder / "m-1000.JSON", "{");
        for (std::size_t index = 1; index < level_format::limit::max_user_stage_files - 2; ++index)
        {
            write_text(folder / ("m-" + std::to_string(1000 + index) + ".json"), "{");
        }
        // Both names sort first but neither is a regular .json file.
        std::filesystem::create_directory(folder / "0-directory.JSON");
        write_text(folder / "0-ignore.txt", "{");
    }
}
