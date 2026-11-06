#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace colourwars_editor
{
    // The editor's one temp-then-rename writer, shared by document Save and
    // stage registration. It is the editor's own copy of the game's
    // replace_file_atomically (game/save/atomic_file.h), split in two because
    // both callers recheck the destination between writing and replacing.
    //
    // A complete sibling of `destination`, named
    // `<destination>.<tag>-<process>-<ticks>-<n><ending>` and opened CREATE_NEW,
    // so no other writer or editor process shares it; written, flushed to disk
    // and closed before the constructor returns. It is deleted on destruction
    // unless it was moved into place or retained. Throws std::system_error
    // naming the step that failed, after deleting what it created.
    class PreparedFile
    {
    public:
        PreparedFile(const std::filesystem::path& destination, std::string_view tag,
            std::string_view ending, std::string_view bytes);
        ~PreparedFile();
        PreparedFile(const PreparedFile&) = delete;
        PreparedFile& operator=(const PreparedFile&) = delete;

        const std::filesystem::path& path() const { return path_; }
        // Keeps the file after destruction (a backup someone must find).
        void retain() { keep_ = true; }
        // Moves the file over `destination`: replace_prepared_file, after which
        // there is nothing left to delete.
        void replace(const std::filesystem::path& destination);

    private:
        std::filesystem::path path_;
        bool keep_ = false;
    };

    // One same-volume MoveFileExW with replace and write-through: `destination`
    // is either untouched or holds all of `prepared`. Throws std::system_error
    // ("Cannot replace <destination>") on failure.
    void replace_prepared_file(const std::filesystem::path& prepared,
        const std::filesystem::path& destination);
}
