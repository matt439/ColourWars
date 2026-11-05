#include "editor/prepared_file.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <system_error>

namespace colourwars_editor
{
    namespace
    {
        [[noreturn]] void throw_last_error(const std::string& what)
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), what);
        }

        std::string shown(const std::filesystem::path& path)
        {
            const std::u8string text = path.u8string();
            return std::string(text.begin(), text.end());
        }
    }

    PreparedFile::PreparedFile(const std::filesystem::path& destination, std::string_view tag,
        std::string_view ending, std::string_view bytes)
    {
        static std::atomic<unsigned long long> sequence{ 0 };
        path_ = destination;
        path_ += "." + std::string(tag) + "-" + std::to_string(GetCurrentProcessId()) + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
            std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)) + std::string(ending);
        HANDLE output = CreateFileW(path_.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output == INVALID_HANDLE_VALUE) throw_last_error("Cannot create a temporary file beside " +
            shown(destination));
        try
        {
            std::size_t offset = 0;
            while (offset < bytes.size())
            {
                const DWORD size = static_cast<DWORD>(std::min(bytes.size() - offset,
                    static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
                DWORD written = 0;
                if (!WriteFile(output, bytes.data() + offset, size, &written, nullptr))
                    throw_last_error("Cannot write all of the temporary file beside " + shown(destination));
                if (written == 0) throw std::system_error(std::make_error_code(std::errc::io_error),
                    "The temporary file beside " + shown(destination) + " stopped accepting data");
                offset += written;
            }
            if (!FlushFileBuffers(output)) throw_last_error("Cannot flush the temporary file beside " +
                shown(destination));
            const BOOL closed = CloseHandle(output);
            output = INVALID_HANDLE_VALUE;
            if (!closed) throw_last_error("Cannot close the temporary file beside " + shown(destination));
        }
        catch (...)
        {
            if (output != INVALID_HANDLE_VALUE) CloseHandle(output);
            DeleteFileW(path_.c_str());
            throw;
        }
    }

    PreparedFile::~PreparedFile()
    {
        if (!keep_) DeleteFileW(path_.c_str());
    }

    void PreparedFile::replace(const std::filesystem::path& destination)
    {
        replace_prepared_file(path_, destination);
    }

    void replace_prepared_file(const std::filesystem::path& prepared, const std::filesystem::path& destination)
    {
        // One Windows rename replaces the destination. Separate old->backup,
        // temp->destination renames leave the file absent between calls, and
        // ReplaceFileW without a backup can also remove it on error 1176.
        if (!MoveFileExW(prepared.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw_last_error("Cannot replace " + shown(destination));
    }
}
