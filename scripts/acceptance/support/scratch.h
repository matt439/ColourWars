#pragma once

// The probes' one scratch policy and their checked whole-file I/O. A probe
// that needs a private folder takes a fresh GUID-named one under %TEMP% and
// removes it, with everything in it, when it is done; a probe whose evidence
// must outlive it is handed a folder by its runner instead and keeps it.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "scripts/acceptance/support/probe.h"

namespace acceptance
{
    // A path for a failure message, as UTF-8. path.string() throws on a name
    // the ANSI code page cannot hold, which would hide the real failure.
    inline std::string utf8_path(const std::filesystem::path& path)
    {
        const std::u8string text = path.u8string();
        return { reinterpret_cast<const char*>(text.data()), text.size() };
    }

    // The whole file, byte for byte. Throws when it cannot be read.
    inline std::string read_text(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.good()) { throw std::runtime_error("Cannot read " + utf8_path(path)); }
        return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
    }

    // Replaces the file with exactly these bytes. Throws when it cannot.
    inline void write_text(const std::filesystem::path& path, std::string_view text)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.close();
        if (!output.good()) { throw std::runtime_error("Cannot write " + utf8_path(path)); }
    }

    // A fresh folder under %TEMP% named `ColourWars-<label>-<GUID>`, created
    // empty. Throws when it cannot be named or created.
    inline std::filesystem::path unique_temporary_directory(std::wstring_view label)
    {
        GUID guid{};
        wchar_t name[40]{};
        require(SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, name, 40) > 0,
            "Cannot name a private probe folder");
        const std::filesystem::path root = std::filesystem::temp_directory_path() /
            (L"ColourWars-" + std::wstring(label) + L"-" + name);
        require(std::filesystem::create_directory(root), "Cannot create " + utf8_path(root));
        return root;
    }

    // A unique_temporary_directory removed, with everything in it, on
    // destruction.
    class ScratchDirectory
    {
    public:
        explicit ScratchDirectory(std::wstring_view label) : root_(unique_temporary_directory(label)) {}
        ~ScratchDirectory()
        {
            std::error_code ignored;
            std::filesystem::remove_all(this->root_, ignored);
        }
        ScratchDirectory(const ScratchDirectory&) = delete;
        ScratchDirectory& operator=(const ScratchDirectory&) = delete;

        const std::filesystem::path& root() const { return this->root_; }

    private:
        std::filesystem::path root_;
    };
}
