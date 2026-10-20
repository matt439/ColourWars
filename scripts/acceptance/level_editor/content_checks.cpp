// Content and catalog checks: blockout aids, and resource names resolved
// against the content catalog.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/blockout_aids.h"
#include "editor/content_catalog.h"
#include "game/paint_face_geometry.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace level_editor_acceptance
{
    void check_blockout_aids(const fs::path& repo, const fs::path& scratch)
    {
        const BlockoutTuning tuning = load_blockout_tuning(repo / "game/content/tuning.json", repo / "editor/movement-baseline.json");
        require(tuning.player_width == 52.0 && tuning.player_height == 120.0 &&
            tuning.measured_movement_matches, "Source tuning no longer matches recorded movement baseline; remeasure guide");
        std::string changed = contents(repo / "game/content/tuning.json");
        const std::size_t size = changed.find("\"size\": { \"x\": 52.0, \"y\": 120.0 }");
        require(size != std::string::npos, "Missing player size fixture");
        changed.replace(size, std::string("\"size\": { \"x\": 52.0, \"y\": 120.0 }").size(),
            "\"size\": { \"x\": 70.0, \"y\": 135.0 }");
        write(scratch / "different-tuning.json", changed);
        const BlockoutTuning different = load_blockout_tuning(scratch / "different-tuning.json", repo / "editor/movement-baseline.json");
        require(different.player_width == 70.0 && different.player_height == 135.0 &&
            !different.measured_movement_matches, "Changed tuning must resize spawns and invalidate measured guide");

        LevelDocument document;
        const std::vector<Entry> initial = document.entries();
        for (std::vector<Entry>::const_reverse_iterator item = initial.rbegin(); item != initial.rend(); ++item)
            if (item->kind == EntryKind::collision || item->kind == EntryKind::decoration) document.erase(*item);
        const Entry platform = document.add_object("StructurePaintable", {100, 100, 100, 100});
        for (const char* face : {"left", "bottom", "right"})
            document.set_property(platform, std::string("/paintable_faces/") + face, "false");
        document.set_property(platform, "/paintable_faces/top", "true");
        require(scoring_area(scoring_faces(document, tuning)) == 100.0 * tuning.paint_thickness,
            "Self geometry must not hide an authored scoring face");
        document.add_object("Structure", {120, 50, 20, 50});
        document.add_object("Structure", {130, 50, 30, 50});
        document.add_object("StructureJumpThrough", {170, 50, 10, 50});
        document.add_object("StructureRampRight", {100, 50, 100, 100});
        document.add_object("Visual", {100, 50, 100, 100});
        const std::vector<ScoringFace> faces = scoring_faces(document, tuning);
        require(faces.size() == 3 && scoring_area(faces) == 50.0 * tuning.paint_thickness,
            "Merge flat blockers, include jump-through, exclude ramps and decorations");
        require(scoring_area(faces, Rect{100, 100, 25, 6}) == 120.0,
            "Region area must clip scoring strips in both dimensions");
        require(scoring_area(faces, Rect{0, 0, 50, 50}) == 0.0, "Disjoint region has scoring area");
        document.set_property(platform, "/paintable_faces/top", "false");
        require(scoring_faces(document, tuning).empty(), "Disabled faces still score");
        document.undo();
        require(scoring_area(scoring_faces(document, tuning)) == scoring_area(faces), "Undo left scoring diagnostics stale");
        const std::vector<PaintFaceRectangle> blockers{{0, 0, 100, 100}, {0, 100, 100, 150}};
        require(exposed_paint_intervals(0, 100, 100, true, false, blockers).empty(),
            "A touching outward blocker must bury the bottom face");
        require(exposed_paint_intervals(0, 100, 0, true, true, blockers).size() == 1,
            "A blocker behind a leading face must leave it exposed");
        std::cout << "PASS tuned spawns, measured-guide invalidation and runtime scoring regions\n";
    }

    namespace
    {
    // The one diagnostic a mutation must produce, naming its field and value.
    void one_unresolved(const LevelDocument& document, const ContentCatalog& catalog,
        const std::string& field, const std::string& value, const std::string& label)
    {
        const std::vector<std::string> found = document.unresolved_resources(catalog);
        require(found.size() == 1, "Expected one unresolved name for " + label + ", found " +
            std::to_string(found.size()));
        require(found.front().find(field) != std::string::npos &&
            found.front().find("'" + value + "'") != std::string::npos,
            "Unresolved-name diagnostic does not name " + field + " '" + value + "': " + found.front());
    }
    }

    void check_resource_names(const fs::path& repo, const fs::path& scratch)
    {
        const fs::path content = repo / "game/content";
        const ContentCatalog catalog = load_content_catalog(content);
        require(catalog.manifest_read && catalog.problems.empty(), "Source content catalog did not load cleanly");
        for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            require(catalog.sprite_frames.contains(sheet) && catalog.sprite_frames.at(sheet).has_value(),
                std::string("Catalog lacks manifest sprite sheet ") + sheet);
        const std::set<std::string>& sheet_1 = *catalog.sprite_frames.at("sprite_sheet_1");
        require(sheet_1.contains("square_white_4") && sheet_1.contains("stage_close_quarters"),
            "Catalog lacks sprite_sheet_1 frames");
        require(!sheet_1.contains("player_right_walk"), "Catalog took an animation strip for a frame");
        require(catalog.sound_effects.contains("paint_exe") && catalog.sound_effects.at("paint_exe") &&
            catalog.sound_effects.at("paint_exe")->contains("music.close_quarters") &&
            catalog.sound_effects.at("paint_exe")->contains("music.end_sting.close_quarters"),
            "Catalog lacks paint_exe music effects");

        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course", "crossfeed" })
        {
            LevelDocument document;
            document.load(content / "levels" / (std::string(name) + ".json"));
            const std::vector<std::string> found = document.unresolved_resources(catalog);
            require(found.empty(), std::string("Shipped level ") + name + " has an unresolved name: " +
                (found.empty() ? "" : found.front()));
        }
        LevelDocument created;
        require(created.unresolved_resources(catalog).empty(), "New-level defaults have an unresolved name");
        LevelDocument templated;
        templated.load(repo / "editor/template.json");
        require(templated.unresolved_resources(catalog).empty(), "The template has an unresolved name");

        // A frame of another manifest sheet resolves only under its own sheet.
        std::string painter_frame;
        for (const std::string& frame : *catalog.sprite_frames.at("painter"))
            if (!sheet_1.contains(frame)) { painter_frame = frame; break; }
        require(!painter_frame.empty(), "No painter-only frame to test sheet scoping");

        const fs::path stage = content / "levels/close_quarters.json";
        struct Mutation { EntryKind kind; const char* path; std::string value; const char* field; };
        const std::vector<Mutation> mutations = {
            { EntryKind::level, "/icon_frame", "not_an_icon", "icon_frame" },
            { EntryKind::level, "/icon_frame", painter_frame, "icon_frame" },
            { EntryKind::level, "/sound/sound_bank_name", "not_a_bank", "sound_bank_name" },
            { EntryKind::level, "/sound/music_name", "music.not_here", "music_name" },
            { EntryKind::level, "/sound/end_sting_name", "music.end_sting.not_here", "end_sting_name" },
            { EntryKind::collision, "/frame_name", "not_a_frame", "frame_name" },
            { EntryKind::collision, "/frame_name", "player_right_walk", "frame_name" },
            { EntryKind::collision, "/frame_name", painter_frame, "frame_name" },
            { EntryKind::collision, "/sheet_name", "not_a_sheet", "sheet_name" },
            { EntryKind::decoration, "/frame_name", "not_a_frame", "frame_name" },
            { EntryKind::decoration, "/sheet_name", "not_a_sheet", "sheet_name" } };
        for (const Mutation& mutation : mutations)
        {
            LevelDocument document;
            document.load(stage);
            const Entry target = entry(document, mutation.kind);
            document.set_property(target, mutation.path, mutation.value);
            require(document.validate().empty(), "An unresolved name must not be a structural error");
            one_unresolved(document, catalog, mutation.field, mutation.value, mutation.path);
            if (mutation.kind != EntryKind::level)
                require(document.unresolved_resources(catalog).front().starts_with(target.label + ": "),
                    "An object diagnostic must start with its object's label");
            document.undo();
            require(document.unresolved_resources(catalog).empty(), "Undo did not clear the unresolved name");
        }
        {
            LevelDocument document;
            document.load(stage);
            const Entry object = entry(document, EntryKind::collision);
            document.set_property(object, "/sheet_name", "painter");
            document.set_property(object, "/frame_name", painter_frame);
            require(document.unresolved_resources(catalog).empty(), "A frame did not resolve in its own sheet");
            document.set_property(entry(document, EntryKind::level), "/sound/music_name", "music.not_here");
            document.set_property(entry(document, EntryKind::level), "/icon_frame", "not_an_icon");
            require(document.unresolved_resources(catalog).size() == 2, "Each unresolved name needs its own line");
        }
        {
            // check-level-editor.ps1 opens this in the GUI and reads its status line.
            LevelDocument document;
            document.load(stage);
            document.set_property(entry(document, EntryKind::level), "/icon_frame", "not_an_icon");
            document.save(scratch / "unresolved_icon.json");
        }

        // Missing, invalid and partial content: problems, never an exception,
        // and only the names that could be read are checked.
        const fs::path partial = scratch / "partial-content";
        fs::create_directories(partial / "textures");
        fs::create_directories(partial / "sounds");
        fs::copy_file(content / "manifest.json", partial / "manifest.json");
        fs::copy_file(content / "textures/sprite_sheet_1.json", partial / "textures/sprite_sheet_1.json");
        write(partial / "textures/painter.json", "{ \"sprite_frames\": [");
        write(partial / "sounds/paint_exe.json",
            "{ \"waves\": [\"a\", \"b\"], \"create_effect_instance_for_each_wave\": false,"
            " \"sound_effect_instances\": [{ \"name\": \"c\", \"wave\": \"a\" }] }");
        const ContentCatalog partial_catalog = load_content_catalog(partial);
        // painter is invalid; menu_paint, weapon_paint, paint_fx and terrain are absent.
        require(partial_catalog.manifest_read && partial_catalog.problems.size() == 5,
            "Each unreadable sheet must be one problem");
        require(!partial_catalog.sprite_frames.at("painter") &&
            partial_catalog.problems.front().find("painter.json") != std::string::npos,
            "An invalid sheet must be reported and left unchecked");
        require(*partial_catalog.sound_effects.at("paint_exe") == std::set<std::string>{ "c" },
            "Waves became effects without create_effect_instance_for_each_wave");
        {
            LevelDocument document;
            document.load(stage);
            const Entry object = entry(document, EntryKind::collision);
            document.set_property(object, "/sheet_name", "weapon_paint");
            document.set_property(object, "/frame_name", "anything");
            const std::vector<std::string> found = document.unresolved_resources(partial_catalog);
            // Music and sting are not "c"; the weapon_paint object is unchecked.
            require(found.size() == 2 && found[0].find("music_name") != std::string::npos &&
                found[1].find("end_sting_name") != std::string::npos,
                "Partial content checked an unreadable sheet or skipped a readable bank");
        }
        write(partial / "sounds/paint_exe.json",
            "{ \"waves\": [\"a\", \"b\"], \"create_effect_instance_for_each_wave\": true,"
            " \"sound_effect_instances\": [{ \"name\": \"c\", \"wave\": \"a\" }] }");
        require(*load_content_catalog(partial).sound_effects.at("paint_exe") ==
            std::set<std::string>{ "a", "b", "c" }, "Each wave must be an effect when the bank asks for it");

        const ContentCatalog missing = load_content_catalog(scratch / "no-content-here");
        require(!missing.manifest_read && missing.problems.size() == 1 &&
            missing.problems.front().find("manifest.json") != std::string::npos,
            "A missing manifest must be one problem");
        LevelDocument unchecked;
        unchecked.set_property(entry(unchecked, EntryKind::level), "/icon_frame", "not_an_icon");
        require(unchecked.unresolved_resources(missing).empty(), "Names were checked without a manifest");
        std::cout << "PASS resource names against manifest sheets and banks, frames not strips, "
            "sheet scoping, partial and missing content\n";
    }
}
