#pragma once

// For probes that touch the game's save. Save finds its folder through
// %LOCALAPPDATA% read by the CRT (game/directory_consts.h), so a probe points
// that variable at a private root for as long as an IsolatedSave lives, and
// never opens the player's own save. Also here: the lock a probe uses to make
// one of the game's whole-file writes fail on purpose, and the check for the
// temporary such a write leaves.

#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace acceptance
{
    // Where Save keeps its file under a LOCALAPPDATA root.
    inline std::filesystem::path save_path_in(const std::filesystem::path& local_app_data)
    {
        return local_app_data / L"ColourWars" / L"save_data.json";
    }

    // Points the CRT's LOCALAPPDATA at a private root and restores the earlier
    // value on destruction. With no root it takes a fresh GUID-named folder
    // under %TEMP%, with an empty ColourWars folder for the save, and removes
    // it, with everything in it, on destruction. A root the caller names (a
    // runner's evidence folder, a profile shared with a relaunched process) is
    // created if missing, with nothing in it, and left in place.
    class IsolatedSave
    {
    public:
        // A fresh root named for the probe: ColourWars-<label>-<GUID>.
        explicit IsolatedSave(const wchar_t* label) : IsolatedSave(label, {}) {}
        // The caller's root, kept.
        explicit IsolatedSave(const std::filesystem::path& retained_root) : IsolatedSave(L"probe", retained_root)
        {
            require(this->retained_, "An IsolatedSave root to keep must be named");
        }
        // The caller's root, kept; an empty one takes a fresh root, as above.
        IsolatedSave(const wchar_t* label, const std::filesystem::path& retained_root) :
            retained_(!retained_root.empty())
        {
            wchar_t* previous = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0, "Cannot read LOCALAPPDATA");
            if (previous != nullptr) { this->previous_ = previous; std::free(previous); }
            if (this->retained_)
            {
                this->root_ = std::filesystem::absolute(retained_root);
                std::filesystem::create_directories(this->root_);
            }
            else
            {
                this->root_ = unique_temporary_directory(label);
                std::filesystem::create_directory(this->save_path().parent_path());
            }
            require(_wputenv_s(L"LOCALAPPDATA", this->root_.c_str()) == 0,
                "Cannot point LOCALAPPDATA at " + this->root_.string());
        }
        ~IsolatedSave()
        {
            _wputenv_s(L"LOCALAPPDATA", this->previous_.c_str());
            if (this->retained_) { return; }
            std::error_code ignored;
            std::filesystem::remove_all(this->root_, ignored);
        }
        IsolatedSave(const IsolatedSave&) = delete;
        IsolatedSave& operator=(const IsolatedSave&) = delete;

        const std::filesystem::path& root() const { return this->root_; }
        std::filesystem::path save_path() const { return save_path_in(this->root_); }
        std::string save_bytes() const { return read_text(this->save_path()); }
        bool temporary_left() const;

        // Replaces the save file with these bytes, creating its folder.
        void write_save(std::string_view text) const
        {
            std::filesystem::create_directories(this->save_path().parent_path());
            write_text(this->save_path(), text);
        }

    private:
        std::filesystem::path root_;
        std::wstring previous_;
        bool retained_ = false;
    };

    // The game writes every file through replace_file_atomically
    // (game/save/atomic_file.h): a uniquely named sibling temporary, then one
    // replacing move. A probe cannot block that temporary by name, so it
    // refuses the move instead.
    //
    // Holds an existing file open for reading, sharing read and write but not
    // delete. Readers still open it and the writer still writes its temporary,
    // but the replacing move over the file fails, so the write fails after
    // serialization, as a real sharing violation would. Write sharing is what
    // lets a probe tell the two writes apart: a writer that truncated the file
    // in place would get through this lock and change the bytes, so a check
    // that the earlier file survived can fail. Released on destruction.
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
            this->handle_ = CreateFileW(this->path_.c_str(), GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
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

    inline bool IsolatedSave::temporary_left() const { return temporary_beside(this->save_path()); }
}
