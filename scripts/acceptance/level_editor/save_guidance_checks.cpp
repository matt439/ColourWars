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
    }

    void check_save_guidance(const fs::path& scratch)
    {
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
