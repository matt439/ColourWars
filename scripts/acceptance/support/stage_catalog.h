#pragma once

// A probe's GameData borrows a StageCatalog, which owns its definitions. A
// probe that loads one registry and offers several stage lists from it builds
// a catalog per list here, rereading the definitions it needs from their files.

#include "engine/assets/json.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/config/stage_catalog.h"
#include "game/config/stage_loading.h"
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string>

namespace acceptance
{
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
        const labrador::JsonDocument manifest =
            labrador::read_json_file((content / "manifest.json").string().c_str());
        const labrador::JsonValue assets = manifest.root().array("assets");
        StageCatalog catalog;
        for (size_t group = 0; group < assets.size(); ++group)
        {
            const labrador::JsonValue asset = assets.at(group);
            if (asset.string("kind") != "level") { continue; }
            const labrador::JsonValue names = asset.array("names");
            for (size_t index = 0; index < names.size(); ++index)
            {
                const std::string name = names.at(index).as_string();
                catalog.add(read_stage_definition(
                    (content / asset.string("directory") / (name + ".json")).string(), name));
            }
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
