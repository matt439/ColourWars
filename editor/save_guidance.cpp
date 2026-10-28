#include "editor/save_guidance.h"
#include "editor/content_catalog.h"
#include "editor/user_stage_save.h"
#include "game/user_stage_contract.h"

#include <algorithm>
#include <cwctype>
#include <stdexcept>

namespace colourwars_editor
{
    namespace
    {
        bool json_file(const std::filesystem::path& path)
        {
            std::wstring extension = path.extension().native();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
            return extension == L".json";
        }

        std::string stage_name(const std::filesystem::path& path)
        {
            const std::u8string bytes = path.stem().u8string();
            return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
        }
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
                check_user_stage_target(*my_stages, destination, catalog.stages);
                return (saved ? L"Open" : L"Save changes, then open") +
                    std::wstring(L" Stage Select from the game's main menu to play it; no rebuild needed.");
            }
            catch (const std::runtime_error&)
            {
                return L"The game cannot discover this file as a user stage. Use Save to My Stages to choose a valid name.";
            }
        }
        if (json_file(destination) && same_stage_folder(destination.parent_path(), source_levels))
        {
            const std::string name = stage_name(destination);
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
