#pragma once

// For probes that make one of the game's whole-file writes fail on purpose and
// check what is left. The game writes every file through
// replace_file_atomically (game/save/atomic_file.h): a uniquely named sibling
// temporary, then one replacing move. A probe cannot block that temporary by
// name, so it refuses the move instead.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <filesystem>
#include <string>
#include <system_error>

namespace acceptance
{
    // Holds an existing file open for reading without delete sharing. Readers
    // still open it, the writer still writes its temporary, but the move over
    // the file fails, so the write fails after serialization, as a real
    // sharing violation would. Released on destruction.
    class ReplacementLock
    {
    public:
        explicit ReplacementLock(std::filesystem::path path) : path_(std::move(path)) {}
        ~ReplacementLock() { this->release(); }
        ReplacementLock(const ReplacementLock&) = delete;
        ReplacementLock& operator=(const ReplacementLock&) = delete;

        // False when already held or when the file cannot be opened (a probe
        // that locks a file it has not written yet tests nothing).
        bool hold()
        {
            if (this->handle_ != INVALID_HANDLE_VALUE) { return false; }
            this->handle_ = CreateFileW(this->path_.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            return this->handle_ != INVALID_HANDLE_VALUE;
        }

        // False when not held.
        bool release()
        {
            if (this->handle_ == INVALID_HANDLE_VALUE) { return false; }
            const bool closed = CloseHandle(this->handle_) != FALSE;
            this->handle_ = INVALID_HANDLE_VALUE;
            return closed;
        }

        const std::filesystem::path& path() const { return this->path_; }

    private:
        std::filesystem::path path_;
        HANDLE handle_ = INVALID_HANDLE_VALUE;
    };

    // Whether a writer's temporary for `path` is still beside it: any
    // `<file>.*.tmp` in its directory, whatever unique part the writer chose.
    inline bool temporary_beside(const std::filesystem::path& path)
    {
        const std::wstring prefix = path.filename().wstring() + L".";
        std::error_code error;
        for (std::filesystem::directory_iterator it(path.parent_path(), error), end;
            !error && it != end; it.increment(error))
        {
            const std::wstring name = it->path().filename().wstring();
            if (name.size() >= prefix.size() + 3 && name.compare(0, prefix.size(), prefix) == 0 &&
                name.compare(name.size() - 4, 4, L".tmp") == 0)
            {
                return true;
            }
        }
        return false;
    }
}
