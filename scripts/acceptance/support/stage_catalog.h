#pragma once

// A probe's GameData borrows a StageCatalog, which owns its definitions. A
// probe that loads one registry and offers several stage lists from it builds
// a catalog per list here, rereading the definitions it needs from their files.

#include "engine/assets/json.h"
#include "engine/core/file_path.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/config/stage_catalog.h"
#include "game/config/stage_loading.h"
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace acceptance
{
    // One name a manifest group lists, and the file it names under the
    // group's directory: `content`/directory/name`extension`.
    struct ManifestEntry
    {
        std::string name;
        std::filesystem::path path;
    };

    // Every name `content`/manifest.json lists under `kind`, in its order,
    // across every group of that kind: what Application::load_manifest walks.
    inline std::vector<ManifestEntry> manifest_entries(const std::filesystem::path& content,
        std::string_view kind, std::string_view extension)
    {
        const labrador::JsonDocument manifest =
            labrador::read_json_file(labrador::path_to_utf8(content / "manifest.json").c_str());
        const labrador::JsonValue assets = manifest.root().array("assets");
        std::vector<ManifestEntry> entries;
        for (size_t group = 0; group < assets.size(); ++group)
        {
            const labrador::JsonValue asset = assets.at(group);
            if (asset.string("kind") != kind) { continue; }
            const labrador::JsonValue names = asset.array("names");
            for (size_t index = 0; index < names.size(); ++index)
            {
                const std::string name = names.at(index).as_string();
                entries.push_back({ name, content / labrador::path_from_utf8(asset.string("directory")) /
                    labrador::path_from_utf8(name + std::string(extension)) });
            }
        }
        return entries;
    }

    // Every stage the manifest lists, in its order, with the level file it
    // names. A probe that claims "every stage" walks this rather than a list
    // of its own, so a stage added to the manifest is covered the day it
    // lands. Levels are never optional: an empty list, or a listed stage whose
    // file is missing, is refused here.
    inline std::vector<ManifestEntry> manifest_level_paths(const std::filesystem::path& content)
    {
        std::vector<ManifestEntry> levels = manifest_entries(content, "level", ".json");
        if (levels.empty())
        {
            throw std::runtime_error("manifest.json lists no level");
        }
        for (const ManifestEntry& level : levels)
        {
            if (!std::filesystem::is_regular_file(level.path))
            {
                throw std::runtime_error("manifest.json lists " + level.name + ", whose level file is missing");
            }
        }
        return levels;
    }

    // The stage `name` as the game lists it, its display name, icon and Random
    // eligibility read from its definition in `levels`, the manifest-loaded
    // registry: a probe that offers a shipped stage names it rather than
    // restating its file, and fails plainly when that stage leaves the game.
    inline Stage shipped_stage(const labrador::Registry<LevelLoadedInfo>& levels, const std::string& name)
    {
        if (!levels.contains(name))
        {
            throw std::runtime_error("No manifest-loaded definition for the stage " + name);
        }
        const LevelLoadedInfo& info = *levels.get(name);
        return Stage{ name, info.display_name(), info.icon_frame(), info.include_in_random() };
    }

    // Lists `stages` in order, each with its own reading of its definition in
    // `levels` when it has one. Each name in `unlisted` that `levels` holds and
    // `stages` does not list is registered without a list entry, as a user
    // stage that left the list stays registered: what a probe that starts a
    // match on a stage its menu does not offer needs.
    inline StageCatalog catalog_of(const labrador::Registry<LevelLoadedInfo>& levels,
        const StageList& stages, std::initializer_list<const char*> unlisted = {})
    {
        // A definition owns its parsed file and cannot be copied, so the
        // catalog's own is read again from the file the shared one came from.
        const auto copy = [&levels](const std::string& name)
        {
            if (!levels.contains(name)) { return std::unique_ptr<LevelLoadedInfo>(); }
            auto info = std::make_unique<LevelLoadedInfo>(levels.get(name)->source_path().c_str());
            info->validate();
            return info;
        };
        const auto listed = [&stages](const std::string& name)
        {
            for (const Stage& stage : stages)
            {
                if (stage.name == name) { return true; }
            }
            return false;
        };
        StageCatalog catalog;
        for (const char* name : unlisted)
        {
            if (!listed(name) && levels.contains(name))
            {
                Stage stage{ name };
                stage.user = true;
                catalog.add(StageDefinition{ std::move(stage), copy(name) });
            }
        }
        catalog.remove_user_stages();
        for (const Stage& stage : stages)
        {
            catalog.add(StageDefinition{ stage, copy(stage.name) });
        }
        return catalog;
    }

    // Every stage `content`/manifest.json lists, in its order, each read as
    // wWinMain's "level" kind reads it: the catalog the game's GameData holds
    // before any user stage joins. A probe whose debug director loads content
    // as the game does (tune.save, reload, a tune that reaches the levels)
    // needs this list, not a hand-picked one: validate_stage_names holds
    // presentation.json's menu_text.stage_names to exactly the manifest's
    // stages, so a subset makes the shipped file look broken.
    inline StageCatalog manifest_catalog(const std::filesystem::path& content)
    {
        StageCatalog catalog;
        for (const ManifestEntry& level : manifest_level_paths(content))
        {
            catalog.add(read_stage_definition(labrador::path_to_utf8(level.path), level.name));
        }
        return catalog;
    }

    // Lists `stages` with no definitions: for a probe that only plans matches.
    inline StageCatalog catalog_of(const StageList& stages)
    {
        StageCatalog catalog;
        for (const Stage& stage : stages)
        {
            catalog.add(StageDefinition{ stage, nullptr });
        }
        return catalog;
    }
}
