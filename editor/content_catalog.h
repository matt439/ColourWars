#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace colourwars_editor
{
    // The resource names a level may use, read from the files the game's manifest
    // loads: the frames (not animation strips) of each manifest sprite sheet, and the
    // effects of each manifest sound-bank definition by the engine's rule (every
    // wave when create_effect_instance_for_each_wave is true, plus each named
    // instance). It is a second reader of those formats, so it only warns; the
    // game's startup validation stays the authority. Depends on rapidjson alone,
    // so the game-side level_format acceptance case can compile it against the
    // engine's own loaders.
    struct ContentCatalog
    {
        // Every manifest name, with its names, or nullopt when its file could not
        // be read (its problem is then in `problems` and its names go unchecked).
        std::map<std::string, std::optional<std::set<std::string>>> sprite_frames;
        std::map<std::string, std::optional<std::set<std::string>>> sound_effects;
        // The manifest's level names: the shipped stages a player's own stage
        // may not be named after (Save to My Stages, ROAD#16).
        std::vector<std::string> stages;
        std::vector<std::string> problems;
        bool manifest_read = false;
    };

    // Never throws for missing or invalid content: each failure is a problem line.
    ContentCatalog load_content_catalog(const std::filesystem::path& content_directory);
}
