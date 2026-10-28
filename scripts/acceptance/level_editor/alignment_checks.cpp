#include "scripts/acceptance/level_editor/fixtures.h"

#include <iostream>
#include <limits>
#include <string>

namespace level_editor_acceptance
{
    namespace
    {
        rapidjson::Value& alignment_object(rapidjson::Document& document, const Entry& selected)
        {
            return document[selected.kind == EntryKind::collision ? "collision_objects" : "non_collision_objects"]
                [static_cast<rapidjson::SizeType>(selected.index)];
        }

        void load_alignment(LevelDocument& document, const fs::path& path, const rapidjson::Document& source)
        {
            write(path, encode(source));
            document.load(path);
            require(!document.dirty() && !document.can_undo() && !document.can_redo(),
                "Alignment fixture did not load at a clean private saved point");
        }

        void check_alignment_edit(LevelDocument& document, const Entry& selected, int grid,
            const rapidjson::Document& expected)
        {
            const std::string before = document.serialize();
            const fs::path path = document.path();
            const std::string disk = contents(path);
            document.align_origin(selected, grid);
            const std::string after = document.serialize();
            require(encode(parse(after)) == encode(expected) && document.dirty() && document.can_undo() &&
                !document.can_redo() && document.path() == path && contents(path) == disk,
                "Alignment changed more than its literal authored coordinates or lost its one-edit saved boundary");
            require(document.undo() && document.serialize() == before && !document.dirty() &&
                !document.can_undo() && document.can_redo() && document.redo() &&
                document.serialize() == after && document.dirty() && document.can_undo() && !document.can_redo(),
                "Alignment did not retain exactly one complete undo/redo transaction");
            document.align_origin(selected, grid);
            require(document.serialize() == after && document.dirty() && document.can_undo() &&
                !document.can_redo() && document.path() == path && contents(path) == disk,
                "Repeating alignment changed an already aligned origin, numeric types or history");
            require(document.undo() && document.serialize() == before && !document.can_undo() &&
                document.redo() && document.serialize() == after && !document.can_redo(),
                "Repeated no-op alignment inserted an invisible history entry");
        }

        template<typename Action>
        void check_alignment_refusal(LevelDocument& document, Action action, const char* message)
        {
            const Entry level = entry(document, EntryKind::level);
            const std::string saved = document.serialize();
            const fs::path path = document.path();
            const std::string disk = contents(path);
            document.set_property(level, "display_name", "Alignment refusal first");
            const std::string first = document.serialize();
            document.set_property(level, "display_name", "Alignment refusal second");
            const std::string second = document.serialize();
            require(document.undo() && document.serialize() == first && document.dirty() &&
                document.can_undo() && document.can_redo(), "Alignment refusal fixture needs both history branches");
            reject_without_change(document, action, message);
            require(document.redo() && document.serialize() == second && !document.can_redo() &&
                document.undo() && document.serialize() == first && document.undo() &&
                document.serialize() == saved && !document.dirty() && !document.can_undo() &&
                document.redo() && document.serialize() == first && document.can_undo() && document.can_redo() &&
                document.path() == path && contents(path) == disk,
                "Rejected alignment changed a complete undo/redo snapshot, saved path or private source bytes");
        }
    }

    void check_alignment(const fs::path& repo, const fs::path& scratch)
    {
        struct GridCase { int grid; double x, y, expected_x, expected_y; };
        const GridCase grids[] = {
            { 1, 113.5, -87.5, 114, -88 }, { 10, 113.5, -87.5, 110, -90 },
            { 25, 113.5, -87.5, 125, -100 }, { 50, 113.5, -87.5, 100, -100 },
            { 100, 113.5, -87.5, 100, -100 },
            { 1, 0.5, -0.5, 1, -1 }, { 10, 5, -5, 10, -10 },
            { 25, 12.5, -12.5, 25, -25 }, { 50, 25, -25, 50, -50 },
            { 100, 50, -50, 100, -100 } };
        std::size_t ordinal = 0;
        for (const GridCase& test : grids)
        {
            LevelDocument seed;
            const Entry selected = seed.add_object("StructurePaintable", { test.x, test.y, 160.25, 80.5 });
            auto source = parse(seed.serialize());
            source.AddMember("future_root", "retain root metadata", source.GetAllocator());
            auto& object = alignment_object(source, selected);
            object.AddMember("future_object", true, source.GetAllocator());
            object["rectangle"].AddMember("future_geometry", 7, source.GetAllocator());
            auto expected = parse(encode(source));
            auto& rectangle = alignment_object(expected, selected)["rectangle"];
            rectangle["x"].SetDouble(test.expected_x); rectangle["y"].SetDouble(test.expected_y);
            LevelDocument document;
            load_alignment(document, scratch / ("alignment-grid-" + std::to_string(ordinal++) + ".json"), source);
            check_alignment_edit(document, selected, test.grid, expected);
            require(equal(document.rectangle(selected), { test.expected_x, test.expected_y, 160.25, 80.5 }),
                "Alignment rounded dimensions or missed a literal positive/negative half-grid tie");
        }

        // A TLBR object must stay TLBR, retaining its width/height and metadata.
        LevelDocument tlbr_seed;
        const Entry tlbr = tlbr_seed.add_object("Visual", { 123.5, 223.5, 160.25, 80.5 });
        auto tlbr_source = parse(tlbr_seed.serialize());
        auto& alternate = alignment_object(tlbr_source, tlbr);
        alternate.RemoveMember("rectangle");
        rapidjson::Value tlbr_fields(rapidjson::kObjectType);
        tlbr_fields.AddMember("left", 123.5, tlbr_source.GetAllocator());
        tlbr_fields.AddMember("top", 223.5, tlbr_source.GetAllocator());
        tlbr_fields.AddMember("right", 283.75, tlbr_source.GetAllocator());
        tlbr_fields.AddMember("bottom", 304.0, tlbr_source.GetAllocator());
        tlbr_fields.AddMember("future_geometry", "retain", tlbr_source.GetAllocator());
        alternate.AddMember("rectangle_tlbr", tlbr_fields, tlbr_source.GetAllocator());
        auto tlbr_expected = parse(encode(tlbr_source));
        auto& expected_edges = alignment_object(tlbr_expected, tlbr)["rectangle_tlbr"];
        expected_edges["left"].SetDouble(125); expected_edges["top"].SetDouble(225);
        expected_edges["right"].SetDouble(285.25); expected_edges["bottom"].SetDouble(305.5);
        LevelDocument tlbr_document;
        load_alignment(tlbr_document, scratch / "alignment-tlbr.json", tlbr_source);
        check_alignment_edit(tlbr_document, tlbr, 25, tlbr_expected);

        for (bool left : { false, true })
        {
            LevelDocument seed;
            const Entry ramp = seed.add_object(left ? "StructureRampLeft" : "StructureRampRight",
                { 113.5, 287.5, 160, 80 });
            auto source = parse(seed.serialize());
            alignment_object(source, ramp)["triangle"].AddMember("future_geometry", "retain", source.GetAllocator());
            auto expected = parse(encode(source));
            auto& triangle = alignment_object(expected, ramp)["triangle"];
            triangle["x1"].SetDouble(left ? 285 : 125); triangle["y1"].SetDouble(380);
            triangle["x2"].SetDouble(left ? 125 : 285); triangle["y2"].SetDouble(380);
            triangle["x3"].SetDouble(left ? 125 : 285); triangle["y3"].SetDouble(300);
            LevelDocument document;
            load_alignment(document, scratch / (left ? "alignment-ramp-left.json" : "alignment-ramp-right.json"), source);
            check_alignment_edit(document, ramp, 25, expected);
            require(equal(document.rectangle(ramp), { 125, 300, 160, 80 }),
                "Ramp alignment changed handedness, dimensions or its geometric origin");
        }

        LevelDocument bounds_seed;
        const Entry camera{ EntryKind::bounds, 1, "Camera bounds" };
        bounds_seed.set_rectangle(camera, { 113.5, 287.5, 160.25, 80.5 });
        auto bounds_source = parse(bounds_seed.serialize());
        auto bounds_expected = parse(encode(bounds_source));
        bounds_expected["level_bounds"]["camera_bounds"]["x"].SetDouble(125);
        bounds_expected["level_bounds"]["camera_bounds"]["y"].SetDouble(300);
        LevelDocument bounds_document;
        load_alignment(bounds_document, scratch / "alignment-bounds.json", bounds_source);
        check_alignment_edit(bounds_document, camera, 25, bounds_expected);

        for (bool team_b : { false, true })
        {
            LevelDocument seed;
            const Entry spawn{ team_b ? EntryKind::spawn_b : EntryKind::spawn_a, 0, "Spawn" };
            seed.set_rectangle(spawn, { team_b ? 2587.5 : 112.5, 1075, 52, 120 });
            auto source = parse(seed.serialize());
            const char* team = team_b ? "team_b" : "team_a";
            source["spawn_locations"][team][0].AddMember("future_spawn", "retain", source.GetAllocator());
            auto expected = parse(encode(source));
            expected["spawn_locations"][team][0]["x"].SetDouble(team_b ? 2600 : 125);
            LevelDocument document;
            load_alignment(document, scratch / (team_b ? "alignment-spawn-b.json" : "alignment-spawn-a.json"), source);
            check_alignment_edit(document, spawn, 25, expected);
        }

        const fs::path tuning = repo / "game/content/tuning.json";
        LevelDocument mover_seed(tuning);
        const Entry mover = mover_seed.add_object("MovingPaintable", { 613, 613, 320, 20 });
        mover_seed.set_property(mover, "motion/phase", "0.25");
        mover_seed.set_property(mover, "motion/drive", "paint");
        mover_seed.set_property(mover, "motion/team_a_heads", "left");
        auto mover_source = parse(mover_seed.serialize());
        alignment_object(mover_source, mover)["motion"].AddMember("future_motion", "retain", mover_source.GetAllocator());
        auto mover_expected = parse(encode(mover_source));
        alignment_object(mover_expected, mover)["rectangle"]["x"].SetDouble(625);
        alignment_object(mover_expected, mover)["rectangle"]["y"].SetDouble(625);
        LevelDocument mover_document(tuning);
        load_alignment(mover_document, scratch / "alignment-mover.json", mover_source);
        check_alignment_edit(mover_document, mover, 25, mover_expected);
        require(equal(mover_document.motion_guide(mover)->anchor, { 625, 625, 320, 20 }) &&
            equal(mover_document.motion_guide(mover)->initial_pose, { 685, 625, 320, 20 }),
            "Mover alignment used its phase pose instead of its authored anchor");

        for (bool left : { false, true })
        {
            LevelDocument seed;
            const Entry slab = seed.add_object(left ? "FixedSlabLeft" : "FixedSlabRight", { 1240.5, 880.5, 1, 1 });
            auto source = parse(seed.serialize());
            alignment_object(source, slab)["slab"].AddMember("future_slab", "retain", source.GetAllocator());
            auto expected = parse(encode(source));
            alignment_object(expected, slab)["slab"]["x"].SetDouble(1250);
            alignment_object(expected, slab)["slab"]["y"].SetDouble(875);
            LevelDocument document;
            load_alignment(document, scratch / (left ? "alignment-slab-left.json" : "alignment-slab-right.json"), source);
            check_alignment_edit(document, slab, 25, expected);
        }

        // A no-op must preserve original JSON number types and a live redo
        // branch, even though ordinary setters use parsed/double values.
        LevelDocument noop_seed;
        const Entry noop = noop_seed.add_object("Structure", { 100, 150, 160, 80 });
        auto noop_source = parse(noop_seed.serialize());
        auto& raw = alignment_object(noop_source, noop)["rectangle"];
        raw["x"].SetInt(100); raw["y"].SetDouble(150); raw["width"].SetInt(160); raw["height"].SetDouble(80);
        LevelDocument noop_document;
        load_alignment(noop_document, scratch / "alignment-noop.json", noop_source);
        check_alignment_refusal(noop_document, [&] { noop_document.align_origin(noop, 0); }, "Alignment accepted a zero grid");
        // Restore the private saved point before preparing the no-op branch.
        const fs::path noop_path = noop_document.path();
        const std::string noop_disk = contents(noop_path);
        noop_document.load(noop_path);
        const std::string noop_saved = noop_document.serialize();
        const Entry level = entry(noop_document, EntryKind::level);
        noop_document.set_property(level, "display_name", "Alignment no-op first");
        const std::string first = noop_document.serialize();
        noop_document.set_property(level, "display_name", "Alignment no-op second");
        const std::string second = noop_document.serialize();
        require(noop_document.undo(), "Alignment no-op fixture lost its redo preparation");
        noop_document.align_origin(noop, 25);
        require(noop_document.serialize() == first && noop_document.can_undo() && noop_document.can_redo(),
            "Already aligned coordinates changed raw numeric types or truncated redo");
        const auto raw_after = parse(noop_document.serialize());
        const auto& kept = raw_after["collision_objects"][static_cast<rapidjson::SizeType>(noop.index)]["rectangle"];
        require(kept["x"].IsInt() && kept["y"].IsDouble() && kept["width"].IsInt() && kept["height"].IsDouble(),
            "No-op alignment changed integer/double field types");
        for (int invalid_grid : { -1, (std::numeric_limits<int>::min)() })
            reject_without_change(noop_document, [&] { noop_document.align_origin(noop, invalid_grid); },
                "Alignment accepted a negative grid");
        reject_without_change(noop_document, [&] { noop_document.align_origin(level, 25); },
            "Alignment accepted level metadata as an editable origin");
        reject_without_change(noop_document, [&] { noop_document.align_origin({ EntryKind::collision, 999999, "stale" }, 25); },
            "Alignment accepted a stale object entry");
        reject_without_change(noop_document, [&] { noop_document.align_origin({ EntryKind::bounds, 999999, "stale" }, 25); },
            "Alignment accepted a stale bounds entry");
        require(noop_document.redo() && noop_document.serialize() == second && noop_document.undo() &&
            noop_document.serialize() == first && noop_document.undo() && noop_document.serialize() == noop_saved &&
            !noop_document.dirty() && !noop_document.can_undo() && noop_document.redo() &&
            noop_document.serialize() == first && noop_document.can_redo() &&
            noop_document.path() == noop_path && contents(noop_path) == noop_disk,
            "No-op/refused alignment changed complete prepared undo/redo snapshots");

        LevelDocument spawn_seed;
        const Entry edge_spawn{ EntryKind::spawn_a, 0, "Team A spawn 1" };
        spawn_seed.set_rectangle(edge_spawn, { 1490, 1075, 52, 120 });
        LevelDocument spawn_refusal;
        load_alignment(spawn_refusal, scratch / "alignment-spawn-refusal.json", parse(spawn_seed.serialize()));
        check_alignment_refusal(spawn_refusal, [&] { spawn_refusal.align_origin(edge_spawn, 25); },
            "Alignment moved a Team A spawn onto the opposing half");

        LevelDocument blocked_seed(tuning);
        const Entry blocked = blocked_seed.add_object("MovingPaintable", { 612, 600, 320, 20 });
        blocked_seed.add_object("Structure", { 548, 500, 8, 20 });
        LevelDocument blocked_document(tuning);
        load_alignment(blocked_document, scratch / "alignment-mover-refusal.json", parse(blocked_seed.serialize()));
        check_alignment_refusal(blocked_document, [&] { blocked_document.align_origin(blocked, 25); },
            "Alignment published a mover whose standing corridor intersects a blocker");

        LevelDocument edge_seed;
        const Entry edge_slab = edge_seed.add_object("FixedSlabRight", { 65375, 500, 1, 1 });
        LevelDocument edge_document;
        load_alignment(edge_document, scratch / "alignment-slab-refusal.json", parse(edge_seed.serialize()));
        check_alignment_refusal(edge_document, [&] { edge_document.align_origin(edge_slab, 100); },
            "Alignment published a slab endpoint beyond its supported range");

        const fs::path roundtrip = scratch / "alignment-roundtrip.json";
        mover_document.save(roundtrip);
        LevelDocument reopened(tuning); reopened.load(roundtrip);
        require(encode(parse(reopened.serialize())) == encode(mover_expected) && !reopened.dirty() &&
            !reopened.can_undo() && !reopened.can_redo(), "Aligned mover metadata/geometry did not survive private save/reload");
        std::cout << "PASS authored-origin alignment: five grids/half ties, rectangle/TLBR/ramp/bounds/spawn/mover/slab, "
            "metadata/types, one transaction, no-op redo, atomic refusals and private roundtrip\n";
    }
}
