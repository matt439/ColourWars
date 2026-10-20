// Synthetic runtime-staging client; no game code, content or application.
#include "engine/app/content_root.h"
#include "engine/core/file_path.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    void require(bool condition, const char* reason)
    {
        if (!condition) { throw std::runtime_error(reason); }
    }

    std::string read_text(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.is_open(), "could not open synthetic text asset");
        std::string text{ std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>() };
        require(!stream.bad(), "could not read synthetic text asset");
        return text;
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 3, "usage: RuntimeStagingExhibit baseline|revised present|absent");
        const std::string_view expected(argv[1]);
        const std::string_view optional_state(argv[2]);
        require(expected == "baseline" || expected == "revised", "unknown required-text expectation");
        require(optional_state == "present" || optional_state == "absent", "unknown optional expectation");

        const std::string executable_root = labrador::executable_directory();
        const auto root_path = labrador::path_from_utf8(executable_root);
        require(root_path.is_absolute(), "engine executable root must be absolute");
        require(std::filesystem::weakly_canonical(std::filesystem::current_path())
            != std::filesystem::weakly_canonical(root_path), "run from the unrelated fixture cwd");
        require(read_text(labrador::path_from_utf8("runtime/required.txt")) == "cwd-decoy\n",
            "the unrelated cwd must contain the misleading decoy file");

        const auto required_path = labrador::path_from_utf8(
            labrador::resolved_under(executable_root, "runtime/required.txt"));
        require(read_text(required_path) == std::string(expected) + "\n",
            "executable-relative required asset did not match this build phase");
        const auto optional_path = labrador::path_from_utf8(
            labrador::resolved_under(executable_root, "runtime/optional.txt"));
        const bool optional_present = std::filesystem::exists(optional_path);
        require(optional_present == (optional_state == "present"),
            "optional asset presence did not match this build phase");
        if (optional_present)
        {
            require(read_text(optional_path) == "optional-note\n", "optional text changed");
        }
        require(!std::filesystem::exists(labrador::path_from_utf8(
            labrador::resolved_under(executable_root, "runtime/not-for-runtime.txt"))),
            "unlisted source content leaked into the runtime mirror");

        std::cout << "PASS runtime staging: required=" << expected
            << "; optional=" << optional_state
            << "; unlisted excluded; executable-relative read defeats cwd decoy\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL runtime staging: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
