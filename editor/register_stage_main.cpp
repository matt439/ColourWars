#include "editor/stage_registration.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    struct Options
    {
        std::filesystem::path repository;
        std::filesystem::path stage;
        bool apply = false;
    };

    std::string shown(const std::filesystem::path& path)
    {
        const std::u8string bytes = path.u8string();
        return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
    }

    void usage(std::ostream& output)
    {
        output << "Usage: ColourWarsRegisterStage --repo <root> --stage <existing-path> [--apply]\n"
            "       ColourWarsRegisterStage --help\n\n"
            "Preview registration of an existing game/content/levels/<name>.json.\n"
            "Relative stage paths are relative to --repo. Preview writes nothing.\n"
            "--apply explicitly applies the previewed manifest and runtime asset entries.\n"
            "Already registered in both lists is a successful no-op.\n"
            "Ordinary editor Save/Save As does not register stages.\n"
            "Exit codes: 0 success/no-op, 2 usage error, 1 validation or apply failure.\n";
    }

    Options options(int argc, wchar_t** argv)
    {
        Options result;
        bool has_repository = false, has_stage = false;
        for (int index = 1; index < argc; ++index)
        {
            const std::wstring_view flag(argv[index]);
            if (flag == L"--apply")
            {
                if (result.apply) throw std::invalid_argument("--apply was supplied twice.");
                result.apply = true;
                continue;
            }
            if (flag == L"--help") throw std::invalid_argument("Use --help by itself.");
            if (flag != L"--repo" && flag != L"--stage")
                throw std::invalid_argument("Unknown argument: " + shown(std::filesystem::path(flag)));
            bool& present = flag == L"--repo" ? has_repository : has_stage;
            if (present) throw std::invalid_argument(flag == L"--repo" ?
                "--repo was supplied twice." : "--stage was supplied twice.");
            if (index + 1 >= argc || std::wstring_view(argv[index + 1]).empty() ||
                std::wstring_view(argv[index + 1]).starts_with(L"--"))
                throw std::invalid_argument(flag == L"--repo" ?
                    "--repo needs a directory." : "--stage needs an existing level path.");
            present = true;
            const std::filesystem::path path(argv[++index]);
            if (flag == L"--repo") result.repository = path;
            else result.stage = path;
        }
        if (!has_repository || !has_stage)
            throw std::invalid_argument("Both --repo and --stage are required.");
        return result;
    }

    void print_preview(const colourwars_editor::StageRegistrationPreview& preview)
    {
        std::cout << "Stage: " << shown(preview.stage) << '\n'
            << "Name: " << preview.name << '\n';
        if (preview.already_registered)
        {
            std::cout << "Already registered in both files; no changes.\n"
                << "  " << shown(preview.manifest.path) << '\n'
                << "  " << shown(preview.runtime_assets.path) << '\n';
        }
        else
        {
            std::cout << "Proposed entries:\n"
                << "  " << shown(preview.manifest.path) << '\n'
                << "    Add to the level asset group's names: \"" << preview.name << "\"\n"
                << "  " << shown(preview.runtime_assets.path) << '\n'
                << "    Add to COLOURWARS_RUNTIME_ASSETS: \"levels/" << preview.name << ".json\"\n";
        }
        std::cout.flush();
        if (!std::cout) throw std::runtime_error("Cannot write the registration preview; no changes applied.");
    }
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && std::wstring_view(argv[1]) == L"--help")
    {
        usage(std::cout);
        return std::cout ? 0 : 1;
    }
    Options request;
    try
    {
        request = options(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Usage error: " << error.what() << "\n\n";
        usage(std::cerr);
        return 2;
    }
    try
    {
        const auto preview = colourwars_editor::preview_stage_registration(request.repository, request.stage);
        print_preview(preview);
        if (preview.already_registered) return 0;
        if (!request.apply)
        {
            std::cout << "Preview only; no files changed. Re-run with --apply to register.\n";
            return std::cout ? 0 : 1;
        }
        const bool applied = colourwars_editor::apply_stage_registration(preview);
        std::cout << (applied ? "Registered in both files. Configure/build the game to stage the level.\n" :
            "Already registered in both files; no changes.\n");
        return std::cout ? 0 : 1;
    }
    catch (const colourwars_editor::StageRegistrationError& error)
    {
        std::cerr << "Registration failed: " << error.what() << '\n';
        for (const auto& path : error.recovery_files())
            std::cerr << "Retained original for recovery: " << shown(path) << '\n';
        return 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Registration failed: " << error.what() << '\n';
        return 1;
    }
}
