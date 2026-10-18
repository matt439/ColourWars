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
    // The complete texts make the two-file change reviewable before opt-in.
    struct StageRegistrationPreview
    {
        std::filesystem::path repository;
        std::filesystem::path stage;
        std::string name;
        std::string stage_original;
        StageRegistrationFile manifest;
        StageRegistrationFile runtime_assets;
        bool already_registered = false;
    };

    // Throws for invalid/outside/linked paths, invalid level/resource content,
    // duplicate or partial registration, unsupported runtime-list syntax, or a
    // pre-existing disagreement between the two complete asset lists. Relative
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

    // Explicit opt-in only. Revalidates the proposal, prepares both candidates
    // and both original backups before mutation, then rechecks original bytes.
    // Returns false for a validated fully registered no-op; true after both
    // replacements. A second-file failure rolls the first back when it is still
    // our candidate. If rollback cannot be completed safely, the exception names
    // retained original backups. This is not crash-atomic across two files.
    bool apply_stage_registration(const StageRegistrationPreview& preview,
        const StageRegistrationReplace& replace = {});
}
