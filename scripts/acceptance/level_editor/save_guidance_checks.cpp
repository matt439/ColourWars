#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/content_catalog.h"
#include "editor/save_guidance.h"
#include "editor/user_stage_save.h"

#include <iostream>
#include <optional>

namespace level_editor_acceptance
{
    namespace
    {
        bool has(const std::wstring& text, const wchar_t* words)
        {
            return text.find(words) != std::wstring::npos;
        }

        void check_user_guidance(const std::wstring& text)
        {
            require(has(text, L"Stage Select") && has(text, L"main menu") && has(text, L"no rebuild needed"),
                "Direct My Stages feedback lost its main-menu rescan workflow");
            require(!has(text, L"register") && !has(text, L"Rebuild the game"),
                "Direct My Stages feedback added source-registration or rebuild instructions");
        }

        void check_unicode_stage_folders(const fs::path& scratch)
        {
            const fs::path root = scratch / L"save-guidance-unicode";
            const fs::path source = root / L"source_\u00e9" / L"levels";
            const fs::path source_alias = root / L"SOURCE_\u00c9" / L"LEVELS";
            struct FolderPair { fs::path folder; fs::path alias; };
            const FolderPair pairs[]{
                { root / L"profile_\u00e9" / L"ColourWars" / L"stages",
                    root / L"PROFILE_\u00c9" / L"colourwars" / L"STAGES" },
                { root / L"profile_\u00e4" / L"ColourWars" / L"stages",
                    root / L"PROFILE_\u00c4" / L"colourwars" / L"STAGES" }
            };
            ContentCatalog catalog;
            catalog.stages = { "registered" };
            catalog.manifest_read = true;
            require(!fs::exists(root), "Unicode comparison fixture needs its own unused scratch subtree");
            require(same_stage_folder(root / L"ascii" / L"stages", root / L"ASCII" / L"STAGES"),
                "Absent ASCII case aliases no longer identify the same stage folder");
            for (const FolderPair& pair : pairs)
            {
                require(!fs::exists(pair.folder) && !fs::exists(pair.alias),
                    "Unicode folder comparison fixture must start before the folder exists");
                require(same_stage_folder(pair.folder, pair.alias) && same_stage_folder(pair.alias, pair.folder),
                    "Not-yet-created Unicode case aliases must identify the same stage folder");
                require(same_stage_folder(pair.folder / L".", fs::path(pair.alias.native() + L"\\")),
                    "Unicode folder aliases lost dot or trailing-separator normalization");
                const fs::path nested = pair.alias / L"nested";
                const fs::path sibling = pair.alias.parent_path() / L"stages_other";
                const fs::path different = root / L"profile_\u00e8" / L"ColourWars" / L"stages";
                for (const fs::path& other : { nested, sibling, different, source_alias })
                {
                    require(!same_stage_folder(pair.folder, other),
                        "Unicode case matching conflated a nested, sibling or genuinely different folder");
                }
                const fs::path chosen = pair.alias / L"case_arena.JSON";
                const UserStageTarget target = check_user_stage_target(pair.folder, chosen, catalog.stages);
                require(target.file == pair.folder / L"case_arena.JSON" && target.name == "case_arena" && !target.replaces,
                    "Unicode folder alias changed the direct target, ASCII stage key or replacement answer");
                for (bool saved : { false, true })
                {
                    check_user_guidance(save_guidance(chosen, source, catalog, pair.folder, saved));
                    const std::wstring known = save_guidance(source_alias / L"registered.JSON",
                        source, catalog, pair.folder, saved);
                    require(has(known, saved ? L"Rebuild the game" : L"then rebuild") && !has(known, L"Stage Select") &&
                        !has(known, L"Register"), "Unicode source-folder alias lost registered-source guidance");
                    require(has(save_guidance(source_alias / L"new_source.json", source, catalog, pair.folder, saved),
                        saved ? L"Register this new source filename" : L"register this new source filename"),
                        "Unicode source-folder alias lost new-name registration guidance");
                    for (const fs::path& other : { nested, sibling, different })
                    {
                        require(has(save_guidance(other / L"case_arena.json", source, catalog, pair.folder, saved),
                            L"does not discover"), "A different Unicode folder claimed direct user-stage discovery");
                    }
                }
                const auto refused = [&](const fs::path& path, const char* reason)
                {
                    std::string diagnostic;
                    try { static_cast<void>(check_user_stage_target(pair.folder, path, catalog.stages)); }
                    catch (const std::exception& error) { diagnostic = error.what(); }
                    require(diagnostic.find(reason) != std::string::npos,
                        "Unicode folder alias weakened or masked the direct-folder/name contract: " + diagnostic);
                    require(has(save_guidance(path, source, catalog, pair.folder, true),
                        reason == std::string("saves directly") ? L"does not discover" : L"cannot discover"),
                        "Unicode alias guidance disagreed with the refused direct-folder/name contract");
                };
                refused(nested / L"case_arena.json", "saves directly");
                refused(sibling / L"case_arena.json", "saves directly");
                refused(pair.alias / L"bad name.json", "game would refuse");
                refused(pair.alias / L"\u00e9.json", "game would refuse");
                refused(pair.alias / L"REGISTERED.JSON", "shipped stage's name");
                refused(pair.alias / L"case_arena.txt", "only .json");
                require(!fs::exists(pair.folder) && !fs::exists(pair.alias) && !fs::exists(source),
                    "Unicode target or feedback inspection created a destination folder");

                fs::create_directories(pair.folder);
                std::error_code error;
                require(fs::equivalent(pair.folder, pair.alias, error) && !error,
                    "Existing Unicode folder case aliases must be verified by the filesystem");
                require(same_stage_folder(pair.folder, pair.alias) && same_stage_folder(pair.alias, pair.folder),
                    "Stage-folder comparison disagreed with existing filesystem aliases");
                LevelDocument document;
                document.set_property(entry(document, EntryKind::level), "/display_name", "Unicode alias arena");
                save_user_stage(document, pair.folder, target);
                const std::string first = contents(target.file);
                const fs::path reopen_path = pair.alias / L"CASE_ARENA.json";
                error.clear();
                require(fs::equivalent(target.file, reopen_path, error) && !error,
                    "Saved Unicode folder alias did not resolve to the actual stage file");
                LevelDocument reopened;
                reopened.load(reopen_path);
                require(reopened.serialize() == document.serialize() && !reopened.dirty(),
                    "A stage saved through a Unicode folder alias did not reopen unchanged");
                reopened.set_property(entry(reopened, EntryKind::level), "/display_name", "Unicode alias ordinary save");
                reopened.save(target.file);
                require(contents(target.file) != first && !reopened.dirty(),
                    "Ordinary Save across Unicode folder aliases did not retain the actual stage file");
                const UserStageTarget replacement = check_user_stage_target(pair.alias, reopen_path, catalog.stages);
                error.clear();
                require(replacement.replaces && replacement.name == "case_arena" &&
                    replacement.file.filename() == target.file.filename() &&
                    fs::equivalent(replacement.file, target.file, error) && !error,
                    "Existing Unicode folder alias lost case-insensitive replacement identity");
                check_user_guidance(save_guidance(reopen_path, source, catalog, pair.folder, true));
                LevelDocument final;
                final.load(reopen_path);
                require(final.serialize() == reopened.serialize(), "Ordinary Unicode alias Save did not reopen its edited data");
            }
            fs::create_directories(source);
            std::error_code error;
            require(fs::equivalent(source, source_alias, error) && !error && same_stage_folder(source, source_alias),
                "Unicode source-folder classification disagreed with filesystem case aliases");
            std::cout << "PASS Unicode stage folders: absent and filesystem-verified case aliases, ASCII/dot/separator "
                "normalization, direct-folder/name refusals, user/source feedback and actual save/reopen\n";
        }
    }

    void check_save_guidance(const fs::path& scratch)
    {
        check_unicode_stage_folders(scratch);
        const fs::path root = scratch / "save-guidance";
        const fs::path source = root / "source" / "game" / "content" / "levels";
        const fs::path user = root / "profile" / "ColourWars" / "stages";
        ContentCatalog catalog;
        catalog.stages = { "registered" };
        catalog.manifest_read = true;
        const auto guidance = [&](const fs::path& path, bool saved)
        { return save_guidance(path, source, catalog, user, saved); };

        require(has(guidance({}, false), L"Save to My Stages") && !fs::exists(root),
            "Unsaved guidance must offer a destination without creating its folder");
        check_user_guidance(guidance(user / "my_arena.json", false));
        check_user_guidance(guidance(user / "." / "my_arena.JSON", true));
        require(!fs::exists(root), "Inspecting valid user-stage guidance created files or folders");

        LevelDocument document;
        const UserStageTarget target = check_user_stage_target(user, user / "my_arena.json", catalog.stages);
        save_user_stage(document, user, target);
        const std::string user_bytes = contents(target.file);
        const fs::path chosen = document.path();
        check_user_guidance(guidance(document.path(), true));
        document.set_property(entry(document, EntryKind::level), "/display_name", "Guidance local edit");
        document.save(document.path());
        check_user_guidance(guidance(document.path(), true));
        check_user_guidance(guidance(document.path(), false));
        require(document.path() == chosen && contents(target.file) != user_bytes && !document.dirty(),
            "Ordinary Save after Save to My Stages did not retain its destination");

        fs::create_directories(source);
        document.save(source / "registered.json");
        const std::wstring existing = guidance(document.path(), true);
        require(has(existing, L"Rebuild the game") && !has(existing, L"Register") &&
            !has(existing, L"Stage Select") && has(guidance(document.path(), false), L"then rebuild"),
            "Known source-level feedback must rebuild without new-name registration or user rescan");
        document.save(source / "new_source.json");
        const std::wstring unregistered = guidance(document.path(), true);
        require(has(unregistered, L"Register this new source filename") && has(unregistered, L"manifest") &&
            has(unregistered, L"runtime assets") && has(unregistered, L"rebuild") &&
            has(guidance(document.path(), false), L"register this new source filename"),
            "A new source filename must retain registration and rebuild guidance");
        const fs::path elsewhere = root / "elsewhere.json";
        document.save(elsewhere);
        for (bool saved : { false, true })
        {
            const std::wstring text = guidance(document.path(), saved);
            require(has(text, L"does not discover this destination") && has(text, L"Save to My Stages") &&
                !has(text, L"no rebuild needed"), "A file saved elsewhere claimed discovery");
        }
        require(has(guidance(source / "nested" / "registered.json", true), L"does not discover"),
            "A source subfolder was treated as a registered direct source level");
        require(has(guidance(user / "nested" / "my_arena.json", true), L"does not discover"),
            "A nested user stage was treated as discoverable");
        for (const fs::path& bad : { user / "bad name.json", user / "REGISTERED.json", user / "my_arena.txt" })
        {
            require(has(guidance(bad, true), L"cannot discover"),
                "My Stages feedback ignored the game's filename/extension/shipped-name contract");
        }

        catalog.manifest_read = false;
        require(has(guidance(user / "my_arena.json", true), L"unchecked") &&
            !has(guidance(user / "my_arena.json", true), L"no rebuild needed") &&
            has(guidance(source / "registered.json", true), L"Check registration"),
            "An unreadable catalog was reported as a verified user or registered source destination");
        require(has(save_guidance(elsewhere, source, catalog, std::nullopt, true), L"does not discover"),
            "An unavailable user folder caused an elsewhere file to claim discovery");
        require(document.path() == elsewhere && !document.dirty(),
            "Feedback construction changed the document destination or dirty state");
        std::cout << "PASS save guidance: direct user rescan after ordinary Save/refresh, source registration/rebuild, "
            "elsewhere/nested refusal, shared names and unavailable catalog/folder\n";
    }
}
