#pragma once

#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace colourwars_editor
{
    struct StageRegistrationFile
    {
        std::filesystem::path path;
        std::string original;
        std::string proposed;
    };

    // A read-only proposal for a level already saved directly in this source
    // tree's game/content/levels. Ordinary document Save never calls this API.
    // The complete texts make the three-file change reviewable before opt-in:
    // the manifest's level names, the runtime asset list, and presentation.json's
    // menu_text.stage_names entry with the level's display_name as its English.
    struct StageRegistrationPreview
    {
        std::filesystem::path repository;
        std::filesystem::path stage;
        std::string name;
        std::string stage_original;
        StageRegistrationFile manifest;
        StageRegistrationFile runtime_assets;
        StageRegistrationFile presentation;
        bool already_registered = false;
    };

    // Throws for invalid/outside/linked paths, invalid level/resource content,
    // duplicate or partial registration (the name in only some of the three),
    // unsupported runtime-list syntax, a pre-existing disagreement between the
    // two complete asset lists or between the manifest's levels and
    // stage_names' keys, a basename starting with '_', a display_name that is
    // not one nonblank line, or an existing stage_names entry that differs
    // from the level's display_name. Relative
    // stage paths are relative to repository. Preview writes nothing.
    // LevelDocument structure and all references in a complete ContentCatalog
    // must pass; the game's startup validation remains the runtime authority.
    StageRegistrationPreview preview_stage_registration(
        const std::filesystem::path& repository,
        const std::filesystem::path& stage);

    class StageRegistrationError : public std::runtime_error
    {
    public:
        StageRegistrationError(std::string message,
            std::vector<std::filesystem::path> recovery_files = {});
        const std::vector<std::filesystem::path>& recovery_files() const { return recovery_files_; }

    private:
        std::vector<std::filesystem::path> recovery_files_;
    };

    // The test seam replaces one prepared sibling file or throws while leaving
    // both paths untouched. The default uses one same-volume MoveFileExW.
    using StageRegistrationReplace = std::function<void(const std::filesystem::path& prepared,
        const std::filesystem::path& destination)>;

    // Explicit opt-in only. Revalidates the proposal, prepares all three
    // candidates and original backups before mutation, then rechecks each
    // file's bytes before each replacement. Returns false for a validated fully
    // registered no-op; true after all three replacements, in the order
    // manifest, runtime assets, presentation. A later failure rolls the files
    // already replaced back, newest first, each only while it is still our
    // candidate. If rollback cannot be completed safely, the exception names the
    // retained original backups. This is not crash-atomic across the files.
    bool apply_stage_registration(const StageRegistrationPreview& preview,
        const StageRegistrationReplace& replace = {});
}
