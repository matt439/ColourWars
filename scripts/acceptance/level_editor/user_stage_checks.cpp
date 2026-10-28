// Save to My Stages (ROAD#16): the folder, the game's name and collision
// rules, the overwrite answer, and that the folder appears only on the save.
// LOCALAPPDATA points at scratch for the whole check and is restored after;
// the player's profile and save are never read or written.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/content_catalog.h"
#include "editor/user_stage_save.h"
#include "game/user_stage_contract.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace level_editor_acceptance
{
    namespace
    {
        std::optional<std::wstring> environment(const wchar_t* name)
        {
            size_t length = 0;
            wchar_t* raw = nullptr;
            const errno_t result = _wdupenv_s(&raw, &length, name);
            const std::unique_ptr<wchar_t, decltype(&std::free)> buffer(raw, &std::free);
            if (result != 0 || buffer == nullptr) return std::nullopt;
            return std::wstring(buffer.get());
        }

        // Restores LOCALAPPDATA however the check ends.
        struct IsolatedProfile
        {
            std::optional<std::wstring> previous = environment(L"LOCALAPPDATA");
            explicit IsolatedProfile(const fs::path& profile)
            {
                require(_wputenv_s(L"LOCALAPPDATA", profile.c_str()) == 0, "Cannot isolate LOCALAPPDATA");
            }
            ~IsolatedProfile() { _wputenv_s(L"LOCALAPPDATA", previous ? previous->c_str() : L""); }
        };

        template<typename Action>
        void refused(Action action, const std::string& reason, const char* message)
        {
            try { action(); }
            catch (const std::exception& error)
            {
                require(std::string(error.what()).find(reason) != std::string::npos,
                    std::string(message) + ": " + error.what());
                return;
            }
            require(false, message);
        }
    }

    void check_user_stage_save(const fs::path& repo, const fs::path& scratch)
    {
        const fs::path profile = scratch / "user-stage-profile";
        fs::remove_all(profile);
        fs::create_directories(profile);
        const IsolatedProfile isolated(profile);

        const fs::path folder = user_stage_folder();
        require(folder == profile / "ColourWars" / "stages" &&
            folder == user_stage_contract::folder_under(profile),
            "Save to My Stages must use the game's %LOCALAPPDATA%\\ColourWars\\stages");
        require(!fs::exists(profile / "ColourWars"), "Finding the folder must not create it");

        const ContentCatalog catalog = load_content_catalog(repo / "game/content");
        require(catalog.manifest_read && catalog.stages.size() == 10 &&
            std::find(catalog.stages.begin(), catalog.stages.end(), "crossfeed") != catalog.stages.end(),
            "The catalog must list the manifest's ten shipped stages");
        const std::vector<std::string>& shipped = catalog.stages;

        // The game's own refusals, before anything is written.
        refused([&] { check_user_stage_target(folder, folder / "Crossfeed.json", shipped); },
            "shipped stage's name ('crossfeed')", "A case-only shipped-name collision must be refused");
        refused([&] { check_user_stage_target(folder, folder / "bad name.json", shipped); },
            user_stage_contract::name_rule(), "A file name with a space must be refused by the game's rule");
        refused([&] { check_user_stage_target(folder, folder / "-leading.json", shipped); },
            user_stage_contract::name_rule(), "A leading hyphen must be refused");
        refused([&] { check_user_stage_target(folder, folder / (std::string(65, 'a') + ".json"), shipped); },
            user_stage_contract::name_rule(), "A 65-character name must be refused");
        refused([&] { check_user_stage_target(folder, folder / L"stäge.json", shipped); },
            user_stage_contract::name_rule(), "A non-ASCII name must be refused");
        refused([&] { check_user_stage_target(folder, folder / "my_arena.txt", shipped); },
            ".json", "A file the game would not read must be refused");
        refused([&] { check_user_stage_target(folder, profile / "my_arena.json", shipped); },
            "Use Save As", "A file outside the folder must be refused");
        refused([&] { check_user_stage_target(folder, folder / "nested" / "my_arena.json", shipped); },
            "Use Save As", "A file in a sub-folder (which the game ignores) must be refused");
        const UserStageTarget longest = check_user_stage_target(folder,
            folder / (std::string(64, 'b') + ".json"), shipped);
        require(longest.name.size() == 64 && !longest.replaces, "A 64-character name is the game's limit and is allowed");
        require(!fs::exists(profile / "ColourWars"), "Checking a target must not create the folder");

        LevelDocument document(repo / "game/content/tuning.json");
        document.load(repo / "game/content/levels/close_quarters.json");
        require(suggested_user_stage_name(document, shipped) == "my_close_quarters",
            "A shipped stage's copy must be offered a name the game accepts");

        const UserStageTarget first = check_user_stage_target(folder,
            fs::path(folder.native() + L"\\.\\my_arena.json"), shipped);
        require(first.name == "my_arena" && !first.replaces, "A free name must save as a new user stage");
        save_user_stage(document, folder, first);
        require(fs::is_regular_file(folder / "my_arena.json") && document.path() == folder / "my_arena.json",
            "The save must write the folder's file and the document must then edit it");
        require(suggested_user_stage_name(document, shipped) == "my_arena",
            "The open user stage must be offered its own name");
        LevelDocument reloaded(repo / "game/content/tuning.json");
        reloaded.load(folder / "my_arena.json");
        require(reloaded.serialize() == document.serialize(), "The saved user stage must reload unchanged");

        // A second save of the same name, in any spelling, replaces that file.
        const UserStageTarget again = check_user_stage_target(folder, folder / "MY_ARENA.json", shipped);
        require(again.replaces && again.file == folder / "my_arena.json" && again.name == "my_arena",
            "An existing user stage must be reported for the overwrite warning, keeping its spelling");
        save_user_stage(document, folder, again);
        std::size_t files = 0;
        for ([[maybe_unused]] const auto& entry : fs::directory_iterator(folder)) ++files;
        require(files == 1, "Replacing a user stage must not leave a second file");

        fs::create_directories(folder / "taken.json");
        refused([&] { check_user_stage_target(folder, folder / "Taken.json", shipped); },
            "is not a file", "A folder in the way of the name must be refused");

        require(!fs::exists(profile / "ColourWars" / "save_data.json"), "The editor must never write the player's save");
        std::cout << "PASS Save to My Stages: game folder and name rules, shipped-name refusal, overwrite warning, "
            "folder created only on save, no save file\n";
    }
}
