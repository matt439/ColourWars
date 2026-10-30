#pragma once

// A probe's GameData borrows a StageCatalog, which owns its definitions. A
// probe that loads one registry and offers several stage lists from it builds
// a catalog per list here, rereading the definitions it needs from their files.

#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/stage_catalog.h"
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
