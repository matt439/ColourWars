// Stage-registration checks: the private-repository fixture, its snapshot and
// helpers, and the four sub-checks check_stage_registration runs.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/stage_registration.h"
#include <Windows.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace level_editor_acceptance
{
    namespace
    {
    struct RegistrationFixture
    {
        fs::path repository, stage, manifest, runtime;
        std::string stage_bytes, manifest_bytes, runtime_bytes;
        std::string last_level_name;

        RegistrationFixture(const fs::path& repo, const fs::path& scratch) :
            repository(scratch / "registration-repository"),
            stage(repository / "game/content/levels/acceptance_arena.json"),
            manifest(repository / "game/content/manifest.json"), runtime(repository / "cmake/runtime_assets.cmake")
        {
            require(!fs::exists(repository), "Registration acceptance needs a fresh private repository");
            const fs::path source_content = repo / "game/content";
            const fs::path copied_content = repository / "game/content";
            fs::create_directories(copied_content);
            fs::create_directories(repository / "cmake");
            // Copy only the manifest's reviewed runtime vocabulary. Source
            // content also contains private legacy masters which are neither
            // runtime input nor appropriate acceptance evidence.
            std::set<fs::path> runtime_files{ "manifest.json", "tuning.json", "presentation.json" };
            const rapidjson::Document source_manifest = parse(contents(source_content / "manifest.json"));
            const std::map<std::string, std::vector<std::string>> extensions{
                { "font", { "spritefont" } }, { "sprite_sheet", { "json", "dds" } },
                { "level", { "json" } }, { "sound_bank", { "json", "xwb" } } };
            for (const rapidjson::Value& group : source_manifest["assets"].GetArray())
            {
                const std::string kind = group["kind"].GetString();
                require(extensions.contains(kind), "Review a new manifest kind before copying registration fixtures");
                for (const rapidjson::Value& name : group["names"].GetArray())
                {
                    if (kind == "level") { last_level_name = name.GetString(); }
                    for (const std::string& extension : extensions.at(kind))
                    {
                        const fs::path relative = (fs::path(group["directory"].GetString()) /
                            (std::string(name.GetString()) + "." + extension)).lexically_normal();
                        require(!relative.is_absolute() && !relative.empty() &&
                            std::none_of(relative.begin(), relative.end(), [](const fs::path& part) { return part == ".."; }),
                            "A runtime fixture path must stay inside its private content directory");
                        require(runtime_files.insert(relative).second, "Duplicate reviewed registration fixture asset");
                    }
                }
            }
            require(!last_level_name.empty() &&
                last_level_name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos,
                "The independent registration byte oracle needs a last level with a plain identifier");
            for (const fs::path& relative : runtime_files)
            {
                fs::create_directories((copied_content / relative).parent_path());
                fs::copy_file(source_content / relative, copied_content / relative);
            }
            fs::copy_file(repo / "cmake/runtime_assets.cmake", runtime);
            stage_bytes = contents(repo / "game/content/levels/close_quarters.json");
            manifest_bytes = contents(manifest);
            runtime_bytes = contents(runtime);
            write(stage, stage_bytes);
        }

        void reset() const
        {
            write(stage, stage_bytes); write(manifest, manifest_bytes); write(runtime, runtime_bytes);
        }
    };

    struct RegistrationSnapshot
    {
        std::string stage, manifest, runtime;
        std::map<fs::path, fs::file_time_type> files;

        explicit RegistrationSnapshot(const RegistrationFixture& fixture) :
            stage(contents(fixture.stage)), manifest(contents(fixture.manifest)), runtime(contents(fixture.runtime))
        {
            for (const fs::directory_entry& file : fs::recursive_directory_iterator(fixture.repository))
                if (file.is_regular_file()) files.emplace(file.path(), file.last_write_time());
        }

        void unchanged(const RegistrationFixture& fixture, bool times = true) const
        {
            require(contents(fixture.stage) == stage && contents(fixture.manifest) == manifest &&
                contents(fixture.runtime) == runtime, "Rejected registration changed an original input file");
            const RegistrationSnapshot after(fixture);
            require(after.files.size() == files.size(), "Registration leaked or removed a private repository file");
            for (const auto& [path, time] : files)
                require(after.files.contains(path) && (!times || after.files.at(path) == time),
                    "Read-only registration changed a file timestamp or repository path");
        }
    };

    template<typename Action>
    void reject_registration(const RegistrationFixture& fixture, Action action, const char* reason,
        bool times = true)
    {
        const RegistrationSnapshot before(fixture);
        bool rejected = false;
        try { action(); }
        catch (const std::exception& error)
        {
            rejected = true;
            require(std::string(error.what()).size() > 0, "Registration failure needs a diagnostic");
        }
        require(rejected, reason);
        before.unchanged(fixture, times);
    }

    void registration_replace(const fs::path& prepared, const fs::path& destination)
    {
        if (!MoveFileExW(prepared.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Acceptance atomic replacement");
    }

    std::string registration_lines(std::string text, const std::string& newline, bool bom)
    {
        if (text.starts_with("\xEF\xBB\xBF")) text.erase(0, 3);
        std::erase(text, '\r');
        if (newline == "\r\n")
        {
            std::string crlf;
            for (const char c : text) { if (c == '\n') crlf += '\r'; crlf += c; }
            text = std::move(crlf);
        }
        return (bom ? std::string("\xEF\xBB\xBF") : std::string()) + text;
    }

    void check_registration_success(RegistrationFixture& fixture)
    {
        for (const std::string newline : { std::string("\n"), std::string("\r\n") })
        {
            for (const bool bom : { false, true })
            {
                std::string manifest = registration_lines(fixture.manifest_bytes, newline, bom);
                manifest.insert(manifest.find('{') + 1, newline +
                    R"(    "_registration_note":{"escaped":"keep\\path\"quoted","number":1e2},)" + newline);
                const std::string prefix = (bom ? std::string("\xEF\xBB\xBF") : std::string()) +
                    "# Keep this unrelated registration note" + newline + "#[=[retain ) ${opaque}]=]" + newline;
                const std::string runtime = prefix + registration_lines(fixture.runtime_bytes, newline, false);
                write(fixture.manifest, manifest); write(fixture.runtime, runtime);
                const RegistrationSnapshot before(fixture);
                const StageRegistrationPreview preview = preview_stage_registration(fixture.repository,
                    "game/content/levels/acceptance_arena.json");
                before.unchanged(fixture);
                require(!preview.already_registered && preview.name == "acceptance_arena" &&
                    preview.stage_original == fixture.stage_bytes && preview.manifest.original == manifest &&
                    preview.runtime_assets.original == runtime, "Preview did not retain exact original bytes and basename");

                // The independent byte oracle adds one literal name after the
                // existing last stage, and one literal path before the first
                // runtime list's closing line. Every other byte must survive.
                std::string expected_manifest = manifest;
                const std::string last = "\"" + fixture.last_level_name + "\"";
                const std::size_t name = expected_manifest.rfind(last);
                require(name != std::string::npos && expected_manifest.find(last) == name,
                    "The independent registration byte oracle needs its last level token exactly once");
                expected_manifest.insert(name + last.size(), "," + newline + "                \"acceptance_arena\"");
                std::string expected_runtime = runtime;
                const std::size_t begin = expected_runtime.find("set(COLOURWARS_RUNTIME_ASSETS");
                const std::size_t closing = expected_runtime.find(newline + ")", begin);
                require(begin != std::string::npos && closing != std::string::npos,
                    "The registration runtime fixture needs its reviewed literal list");
                expected_runtime.insert(closing + newline.size(), "    levels/acceptance_arena.json" + newline);
                require(preview.manifest.proposed == expected_manifest && preview.runtime_assets.proposed == expected_runtime,
                    "Registration changed bytes outside its single name/path append, including BOM, comments or line endings");

                int replacements = 0;
                require(apply_stage_registration(preview, [&](const fs::path& prepared, const fs::path& destination)
                {
                    if (++replacements == 1)
                    {
                        int candidates = 0, originals = 0;
                        for (const fs::directory_entry& file : fs::recursive_directory_iterator(fixture.repository))
                        {
                            const std::string filename = file.path().filename().string();
                            if (filename.find(".registration-") == std::string::npos) continue;
                            const std::string bytes = contents(file.path());
                            if (filename.ends_with("-candidate"))
                            {
                                ++candidates;
                                require(bytes == expected_manifest || bytes == expected_runtime, "Prepared candidate bytes are incomplete");
                            }
                            if (filename.ends_with("-original"))
                            {
                                ++originals;
                                require(bytes == manifest || bytes == runtime, "Prepared original backup bytes are incomplete");
                            }
                        }
                        require(candidates == 2 && originals == 2 && contents(fixture.manifest) == manifest &&
                            contents(fixture.runtime) == runtime, "Both complete candidates and originals must exist before the first replacement");
                    }
                    require(fs::equivalent(destination, replacements == 1 ? fixture.manifest : fixture.runtime),
                        "Registration replaced a file outside the two reviewed targets");
                    registration_replace(prepared, destination);
                }) && replacements == 2 && contents(fixture.manifest) == expected_manifest &&
                    contents(fixture.runtime) == expected_runtime && contents(fixture.stage) == fixture.stage_bytes,
                    "Opt-in registration did not apply exactly the previewed two-file append");
                const RegistrationSnapshot registered(fixture);
                const StageRegistrationPreview again = preview_stage_registration(fixture.repository, fixture.stage);
                require(again.already_registered && again.manifest.proposed == expected_manifest &&
                    again.runtime_assets.proposed == expected_runtime &&
                    !apply_stage_registration(again, [](const fs::path&, const fs::path&)
                        { throw std::runtime_error("A registered no-op must not replace files"); }),
                    "Fully registered content must be a validated no-op");
                registered.unchanged(fixture);
            }
        }
        fixture.reset();
        std::string quoted = fixture.runtime_bytes;
        const std::size_t asset = quoted.find("    manifest.json");
        require(asset != std::string::npos, "Quoted-asset fixture needs its first runtime asset");
        quoted.replace(asset, std::string("    manifest.json").size(), "    \"manifest.json\"");
        write(fixture.runtime, quoted);
        const RegistrationSnapshot before_quoted(fixture);
        const StageRegistrationPreview quoted_preview = preview_stage_registration(fixture.repository, fixture.stage);
        before_quoted.unchanged(fixture);
        std::string without_append = quoted_preview.runtime_assets.proposed;
        const std::string newline = quoted.find("\r\n") == std::string::npos ? "\n" : "\r\n";
        const std::string added = "    levels/acceptance_arena.json" + newline;
        const std::size_t append = without_append.find(added);
        require(append != std::string::npos, "A canonical quoted asset prevented the stage append");
        without_append.erase(append, added.size());
        require(without_append == quoted && apply_stage_registration(quoted_preview) &&
            contents(fixture.runtime) == quoted_preview.runtime_assets.proposed,
            "Canonical quoted asset arguments must remain supported and retain their exact bytes");
        fixture.reset();
    }

    void check_registration_refusals(RegistrationFixture& fixture, const fs::path& scratch)
    {
        const StageRegistrationPreview preview = preview_stage_registration(fixture.repository, fixture.stage);
        for (const bool manifest_only : { false, true })
        {
            write(manifest_only ? fixture.manifest : fixture.runtime,
                manifest_only ? preview.manifest.proposed : preview.runtime_assets.proposed);
            reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
                "A partially registered stage was accepted");
            fixture.reset();
        }
        for (const bool manifest_alias : { false, true })
        {
            std::string manifest = preview.manifest.proposed;
            std::string runtime = preview.runtime_assets.proposed;
            std::string& aliased = manifest_alias ? manifest : runtime;
            const std::size_t at = aliased.find("acceptance_arena");
            require(at != std::string::npos, "Missing newly registered stage for case-alias check");
            aliased.replace(at, std::string("acceptance_arena").size(), "ACCEPTANCE_ARENA");
            write(fixture.manifest, manifest); write(fixture.runtime, runtime);
            reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
                "A stage identifier or runtime path with different case was accepted");
            fixture.reset();
        }
        const auto bad_runtime = [&](const std::string& bytes)
        {
            write(fixture.runtime, bytes);
            reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
                "Malformed, duplicate, aliased or nonliteral runtime assets were accepted");
            fixture.reset();
        };
        for (const char* replacement : { "${UNREVIEWED}", "levels/a.json;levels/b.json", "[=[manifest.json]=]",
            "\"textures\\\\sheet.json\"", "../manifest.json", "/manifest.json", "manifest.json\n    MANIFEST.JSON" })
        {
            std::string bytes = fixture.runtime_bytes;
            const std::size_t at = bytes.find("    manifest.json");
            require(at != std::string::npos, "The runtime refusal fixture needs manifest.json");
            bytes.replace(at, std::string("    manifest.json").size(), std::string("    ") + replacement);
            bad_runtime(bytes);
        }
        bad_runtime(fixture.runtime_bytes + "\nlist(APPEND COLOURWARS_RUNTIME_ASSETS extra.json)\n");
        bad_runtime(fixture.runtime_bytes + "\nset(COLOURWARS_RUNTIME_ASSETS manifest.json)\n");
        bad_runtime(fixture.runtime_bytes + "\nset(NEW_UNREVIEWED_ASSETS extra.json)\n");
        bad_runtime("set(COLOURWARS_RUNTIME_ASSETS manifest.json\n");
        bad_runtime(fixture.runtime_bytes + "\n#[=[ unclosed bracket comment\n");
        {
            std::string quoted_command = fixture.runtime_bytes;
            const std::size_t command = quoted_command.find("set(COLOURWARS_RUNTIME_ASSETS");
            require(command != std::string::npos, "Quoted-command fixture needs the runtime declaration");
            quoted_command.replace(command, 3, "\"set\"");
            bad_runtime(quoted_command);
            std::string quoted_variable = fixture.runtime_bytes;
            const std::size_t variable = quoted_variable.find("COLOURWARS_RUNTIME_ASSETS");
            quoted_variable.replace(variable, std::string("COLOURWARS_RUNTIME_ASSETS").size(),
                "\"COLOURWARS_RUNTIME_ASSETS\"");
            bad_runtime(quoted_variable);
            std::string quoted_opening = fixture.runtime_bytes;
            quoted_opening.replace(command + 3, 1, "\"(\"");
            bad_runtime(quoted_opening);
            std::string quoted_closing = fixture.runtime_bytes;
            const std::size_t closing = quoted_closing.find(')', command);
            require(closing != std::string::npos, "Quoted-closing fixture needs a list terminator");
            quoted_closing.replace(closing, 1, "\")\"");
            bad_runtime(quoted_closing);
        }

        for (const char* replacement : { "\"movement_course\", \"MOVEMENT_COURSE\"", "null", "17" })
        {
            std::string bytes = fixture.manifest_bytes;
            const std::string token = "\"movement_course\"";
            bytes.replace(bytes.rfind(token), token.size(), replacement);
            write(fixture.manifest, bytes);
            reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
                "A case-alias or malformed manifest name was accepted");
            fixture.reset();
        }
        write(fixture.manifest, "{broken");
        reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
            "Malformed manifest JSON was accepted");
        fixture.reset();

        for (const wchar_t* filename : { L"bad name.json", L"-leading.json", L"bad.dot.json", L"bad_\x00e9.json", L"upper.JSON" })
        {
            const fs::path invalid = fixture.stage.parent_path() / filename;
            write(invalid, fixture.stage_bytes);
            reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, invalid); },
                "An invalid stage basename or noncanonical extension was accepted");
        }
        const fs::path outside = scratch / "registration-outside.json";
        write(outside, fixture.stage_bytes);
        reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, outside); },
            "A stage outside the private repository content directory was accepted");
        fs::create_directory(fixture.stage.parent_path() / "nested");
        const fs::path nested = fixture.stage.parent_path() / "nested/arena.json";
        write(nested, fixture.stage_bytes);
        reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, nested); },
            "A nested stage path was accepted instead of a direct levels child");

        write(fixture.stage, "{}");
        reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
            "An invalid level document was accepted for registration");
        fixture.reset();
        rapidjson::Document unresolved = parse(fixture.stage_bytes);
        unresolved["icon_frame"].SetString("registration_missing_frame", unresolved.GetAllocator());
        write(fixture.stage, encode(unresolved));
        reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
            "A structurally valid stage with an unresolved resource was accepted");
        fixture.reset();
        const fs::path sheet = fixture.repository / "game/content/textures/terrain.json";
        const std::string sheet_original = contents(sheet);
        write(sheet, "{\"sprite_frames\":null}");
        reject_registration(fixture, [&] { preview_stage_registration(fixture.repository, fixture.stage); },
            "Registration accepted an incomplete content catalog");
        require(contents(sheet) == "{\"sprite_frames\":null}", "A refused catalog validation rewrote the private sheet");
        write(sheet, sheet_original);
    }

    void check_registration_stale(RegistrationFixture& fixture)
    {
        for (int target = 0; target < 3; ++target)
        {
            const StageRegistrationPreview preview = preview_stage_registration(fixture.repository, fixture.stage);
            const fs::path changed = target == 0 ? fixture.stage : target == 1 ? fixture.manifest : fixture.runtime;
            write(changed, contents(changed) + "\n");
            int replacements = 0;
            reject_registration(fixture, [&] { apply_stage_registration(preview,
                [&](const fs::path&, const fs::path&) { ++replacements; }); },
                "Apply accepted input bytes changed since preview");
            require(replacements == 0, "A stale preview attempted a replacement");
            fixture.reset();
        }
        for (int field = 0; field < 8; ++field)
        {
            StageRegistrationPreview preview = preview_stage_registration(fixture.repository, fixture.stage);
            if (field == 0) preview.manifest.proposed += " ";
            if (field == 1) preview.runtime_assets.proposed += " ";
            if (field == 2) preview.name = "different_arena";
            if (field == 3) preview.already_registered = true;
            if (field == 4) preview.manifest.path = fixture.stage;
            if (field == 5) preview.stage_original += " ";
            if (field == 6) preview.manifest.original += " ";
            if (field == 7) preview.runtime_assets.original += " ";
            reject_registration(fixture, [&] { apply_stage_registration(preview); },
                "Apply accepted a modified registration proposal or target");
        }
    }

    void check_registration_failures(RegistrationFixture& fixture)
    {
        StageRegistrationPreview preview = preview_stage_registration(fixture.repository, fixture.stage);
        int calls = 0;
        reject_registration(fixture, [&] { apply_stage_registration(preview,
            [&](const fs::path&, const fs::path&) { ++calls; throw std::runtime_error("injected first replacement failure"); }); },
            "An injected first replacement failure succeeded");
        require(calls == 1, "First replacement failure attempted a second file");
        calls = 0;
        reject_registration(fixture, [&] { apply_stage_registration(preview,
            [&](const fs::path& prepared, const fs::path& destination)
            {
                if (++calls == 2) throw std::runtime_error("injected second replacement failure");
                registration_replace(prepared, destination);
            }); }, "A second replacement failure did not roll the first file back", false);
        require(calls == 3, "Second-file failure did not invoke exactly one first-file rollback");

        // Actual Win32 denial of the second replacement, after both candidates
        // have been prepared, must restore the first file and clean siblings.
        const HANDLE locked = CreateFileW(fixture.runtime.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(locked != INVALID_HANDLE_VALUE, "Could not lock the private runtime list");
        try
        {
            reject_registration(fixture, [&] { apply_stage_registration(preview); },
                "Registration replaced a runtime list without delete sharing", false);
        }
        catch (...) { CloseHandle(locked); throw; }
        CloseHandle(locked);
        const DWORD attributes = GetFileAttributesW(fixture.runtime.c_str());
        require(attributes != INVALID_FILE_ATTRIBUTES &&
            SetFileAttributesW(fixture.runtime.c_str(), attributes | FILE_ATTRIBUTE_READONLY),
            "Could not mark the private runtime list read-only");
        try
        {
            reject_registration(fixture, [&] { apply_stage_registration(preview); },
                "Registration replaced a read-only runtime list", false);
        }
        catch (...) { SetFileAttributesW(fixture.runtime.c_str(), attributes); throw; }
        require(SetFileAttributesW(fixture.runtime.c_str(), attributes) != 0,
            "Could not restore private runtime-list attributes");

        // This intentionally leaves a partial result only in the newly copied
        // test repository, then proves recovery from the two named originals.
        calls = 0;
        std::vector<fs::path> recovery;
        try
        {
            apply_stage_registration(preview, [&](const fs::path& prepared, const fs::path& destination)
            {
                if (++calls != 1) throw std::runtime_error("injected second/rollback replacement failure");
                registration_replace(prepared, destination);
            });
        }
        catch (const StageRegistrationError& error)
        {
            recovery = error.recovery_files();
            require(std::string(error.what()).find("Original backups retained") != std::string::npos,
                "Rollback failure did not explain the retained-original recovery path");
        }
        require(calls == 3 && recovery.size() == 2 && contents(fixture.manifest) == preview.manifest.proposed &&
            contents(fixture.runtime) == preview.runtime_assets.original,
            "An injected rollback failure did not expose the exact partial result and two original backups");
        require(fs::equivalent(recovery[0].parent_path(), fixture.manifest.parent_path()) &&
            fs::equivalent(recovery[1].parent_path(), fixture.runtime.parent_path()) &&
            recovery[0].filename().string().ends_with("-original") && recovery[1].filename().string().ends_with("-original") &&
            contents(recovery[0]) == preview.manifest.original && contents(recovery[1]) == preview.runtime_assets.original,
            "Retained recovery paths do not contain both byte-exact original files beside their private targets");
        registration_replace(recovery[0], fixture.manifest);
        registration_replace(recovery[1], fixture.runtime);
        require(contents(fixture.manifest) == fixture.manifest_bytes && contents(fixture.runtime) == fixture.runtime_bytes,
            "Restoring the named private backups did not recover both original lists");
        for (const fs::directory_entry& file : fs::recursive_directory_iterator(fixture.repository))
            require(file.path().filename().wstring().find(L".registration-") == std::wstring::npos,
                "A completed registration/recovery left an unreported sibling file");
    }
    }

    void check_stage_registration(const fs::path& repo, const fs::path& scratch)
    {
        RegistrationFixture fixture(repo, scratch);
        check_registration_success(fixture);
        check_registration_refusals(fixture, scratch);
        check_registration_stale(fixture);
        check_registration_failures(fixture);
        std::cout << "PASS stage registration: read-only exact previews; LF/CRLF/BOM appends and validated no-op; "
            "partial/aliased/malformed/nonliteral lists, outside/invalid levels and incomplete resources refused; "
            "stale inputs/proposals, first/second replacement failures, locked/read-only second files, "
            "and byte-exact named-backup recovery in a fresh private repository\n";
    }
}
