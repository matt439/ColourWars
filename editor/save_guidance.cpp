#include "editor/save_guidance.h"
#include "editor/content_catalog.h"
#include "editor/path_text.h"
#include "editor/user_stage_save.h"
#include "game/contracts/level_format.h"
#include "game/contracts/user_stage_contract.h"

#include <algorithm>
#include <stdexcept>
#include <system_error>

namespace colourwars_editor
{
    namespace
    {
        // Only the number of files before the chosen name determines whether
        // the game reads it. The file can be new or an existing replacement.
        std::wstring user_stage_scan_warning(const std::filesystem::path& file)
        {
            namespace fs = std::filesystem;
            constexpr std::size_t limit = level_format::limit::max_user_stage_files;
            const fs::path folder = file.parent_path();
            std::error_code error;
            if (!fs::exists(folder, error) && !error) return {};

            std::size_t earlier = 0;
            for (fs::directory_iterator entries(folder, error); !error && entries != fs::directory_iterator();
                entries.increment(error))
            {
                if (user_stage_contract::is_stage_file(*entries) &&
                    user_stage_contract::filename_less(entries->path(), file) && ++earlier == limit)
                {
                    return L"This file sorts after the first " + std::to_wstring(limit) +
                        L" .json files. Move earlier .json files out of My Stages, then reopen Stage Select from the main menu.";
                }
            }
            return error ? L"The editor could not check which files Stage Select will read." : L"";
        }

        // UTF-8 for a status line; std::filesystem does the conversion.
        std::wstring widen(const std::string& text)
        {
            return utf8_path(text).wstring();
        }
    }

    std::wstring user_stage_replacement_prompt(const UserStageTarget& target)
    {
        const std::wstring warning = user_stage_scan_warning(target.file);
        return L"Replace your stage '" + widen(target.name) + L"' in My Stages?\n\n" +
            (warning.empty() ? L"The game plays the new version the next time Stage Select opens from the main menu." : warning);
    }

    std::wstring stage_name_warning(const std::filesystem::path& destination,
        const std::filesystem::path& source_levels, const ContentCatalog& catalog,
        const std::string& display_name)
    {
        if (destination.empty() || !catalog.manifest_read || !user_stage_contract::has_json_extension(destination) ||
            !same_stage_folder(destination.parent_path(), source_levels))
            return {};
        const std::string name = path_text(destination.stem());
        const auto registered = std::find_if(catalog.stages.begin(), catalog.stages.end(),
            [&name](const std::string& stage) { return user_stage_contract::same_name(stage, name); });
        if (registered == catalog.stages.end()) return {};
        // Only the title goes unchecked; resource names and guidance stand.
        if (!catalog.stage_names_read)
            return L"Stage title unchecked: " + (catalog.stage_names_problem.empty() ?
                std::wstring(L"presentation.json's menu_text.stage_names was not read") :
                widen(catalog.stage_names_problem));
        const auto entry = catalog.stage_names.find(*registered);
        const std::wstring id = widen(*registered);
        if (entry == catalog.stage_names.end())
            return L"presentation.json's menu_text.stage_names has no '" + id +
                L"'; the game will not start until it names this stage.";
        if (entry->second == display_name) return {};
        return L"presentation.json's menu_text.stage_names." + id + L" says '" + widen(entry->second) +
            L"'; the game will not start until it and display_name agree.";
    }

    std::wstring save_guidance(const std::filesystem::path& destination,
        const std::filesystem::path& source_levels, const ContentCatalog& catalog,
        const std::optional<std::filesystem::path>& my_stages, bool saved)
    {
        if (destination.empty())
        {
            return L"Save to My Stages to play without rebuilding, or save in source levels and register new filenames.";
        }
        if (my_stages && same_stage_folder(destination.parent_path(), *my_stages))
        {
            if (!catalog.manifest_read)
            {
                return L"User-stage discovery is unchecked until the source manifest can be read. See Help before playing it.";
            }
            try
            {
                const UserStageTarget target = check_user_stage_target(*my_stages, destination, catalog.stages);
                const std::wstring warning = user_stage_scan_warning(target.file);
                if (!warning.empty()) return warning;
                return (saved ? L"Open" : L"Save changes, then open") +
                    std::wstring(L" Stage Select from the game's main menu to play it; no rebuild needed.");
            }
            catch (const std::runtime_error&)
            {
                return L"The game cannot discover this file as a user stage. Use Save to My Stages to choose a valid name.";
            }
        }
        if (user_stage_contract::has_json_extension(destination) && same_stage_folder(destination.parent_path(), source_levels))
        {
            const std::string name = path_text(destination.stem());
            const bool registered = catalog.manifest_read && std::any_of(catalog.stages.begin(), catalog.stages.end(),
                [&name](const std::string& stage) { return user_stage_contract::same_name(stage, name); });
            if (registered)
            {
                return saved ? L"Rebuild the game to play changes." :
                    L"Save changes, then rebuild the game to play them.";
            }
            if (catalog.manifest_read)
            {
                return saved ? L"Register this new source filename in the manifest and runtime assets (see Help), then rebuild the game." :
                    L"Save changes, register this new source filename (see Help), then rebuild the game.";
            }
            return saved ? L"Check registration of this source filename (see Help), then rebuild the game." :
                L"Save changes, check registration of this source filename (see Help), then rebuild the game.";
        }
        return L"The game does not discover this destination. Use Save to My Stages, or save in source levels and register it.";
    }
}
