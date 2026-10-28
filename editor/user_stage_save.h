#pragma once

#include "editor/level_document.h"

#include <filesystem>
#include <string>
#include <vector>

// Save to My Stages (ROAD#16): writes the open level straight into the folder
// the game reads a player's own stages from, under the game's own name rules
// (game/user_stage_contract.h). The window shell asks for the file and the
// overwrite; this decides and writes, so acceptance can check it without a
// dialog. Nothing here reads or writes the player's save.
namespace colourwars_editor
{
    // Windows folder identity used by destination validation and save guidance.
    // Resolves relative segments and compares without case; neither folder needs
    // to exist and nothing is created.
    bool same_stage_folder(const std::filesystem::path& first, const std::filesystem::path& second);

    // %LOCALAPPDATA%\ColourWars\stages, the game's user_stage_directory. Throws
    // when LOCALAPPDATA is unset: the game then reads beside its own
    // executable, which the editor cannot know. Never creates the folder.
    std::filesystem::path user_stage_folder();

    struct UserStageTarget
    {
        std::filesystem::path file;
        std::string name;
        // A stage of this name, in any spelling, is already in the folder;
        // `file` is then that file, keeping its spelling.
        bool replaces = false;
    };

    // Checks `chosen` as a destination: directly in `folder`, a .json file
    // whose name the game accepts and that no shipped stage has, compared
    // without case. Throws naming the reason. Reads the folder, never writes.
    UserStageTarget check_user_stage_target(const std::filesystem::path& folder,
        const std::filesystem::path& chosen, const std::vector<std::string>& shipped);

    // A file name to offer in the dialog: the open file's stem when the game
    // would accept it, else "my_" before it, else "my_stage".
    std::string suggested_user_stage_name(const LevelDocument& document,
        const std::vector<std::string>& shipped);

    // Creates the folder if it is missing (only here, on the explicit save)
    // and saves the document there; the document then edits that file.
    void save_user_stage(LevelDocument& document, const std::filesystem::path& folder,
        const UserStageTarget& target);
}
