#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/blockout_aids.h"
#include "editor/slab_authoring.h"
#include <rapidjson/schema.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

namespace level_editor_acceptance
{
    namespace
    {
        std::vector<ScoringFace> faces_for(const LevelDocument& document, const Entry& wanted)
        {
            BlockoutTuning tuning; tuning.paint_thickness = 12.0;
            std::vector<ScoringFace> result;
            for (const auto& face : scoring_faces(document, tuning))
                if (face.entry.index == wanted.index && face.entry.kind == wanted.kind) { result.push_back(face); }
            return result;
        }
    }

    void check_fixed_slabs(const fs::path& repo, const fs::path& scratch)
    {
        LevelDocument document(repo / "game/content/tuning.json");
        const Entry right = document.add_object("FixedSlabRight", { 1240, 880, 1, 1 });
        const Entry left = document.add_object("FixedSlabLeft", { 2600, 800, 1, 1 });
        const auto guide = *document.slab_guide(right);
        const auto mirror = *document.slab_guide(left);
        require(std::abs(guide[0].x - 1240) < 0.001 && std::abs(guide[0].y - 880) < 0.001 &&
            std::abs(guide[1].x - 1400) < 0.001 && std::abs(guide[1].y - 800) < 0.001,
            "Slab guide lost the actual top support segment");
        require(std::abs(guide[2].x - guide[1].x - 24 / std::sqrt(5.0)) < 0.001 &&
            std::abs(guide[2].y - guide[1].y - 48 / std::sqrt(5.0)) < 0.001,
            "Slab thickness was extruded vertically instead of along the normal");
        for (std::size_t i = 0; i < guide.size(); ++i)
        {
            const std::size_t reflected[] = { 1, 0, 3, 2 };
            require(std::abs(guide[i].x + mirror[reflected[i]].x - 4000) < 0.001 &&
                std::abs(guide[i].y - mirror[reflected[i]].y) < 0.001,
                "Left/right fixed slabs are not mirrored geometry");
        }
        const auto source = parse(document.serialize());
        const auto& slab = source["collision_objects"][static_cast<rapidjson::SizeType>(right.index)];
        require(!slab.HasMember("rectangle") && !slab.HasMember("sheet_name") && !slab.HasMember("colour") &&
            std::string(slab["type"].GetString()) == "FixedSlabPaintable" &&
            std::string(slab["material"].GetString()) == "eraser",
            "Slab palette added unsupported generic geometry or sprite fields");
        const std::string before = document.serialize();
        const Rect bounds = document.rectangle(right);
        Rect moved = bounds; moved.x += 50; moved.y += 25;
        const auto preview = *document.slab_guide(right, moved);
        require(document.serialize() == before && std::abs(preview[0].x - guide[0].x - 50) < 0.001,
            "Provisional slab guide mutated source or failed to translate");
        document.set_rectangle(right, moved);
        const auto shifted = *document.slab_guide(right);
        require(std::abs(shifted[0].x - preview[0].x) < 0.001 &&
            std::abs(shifted[0].y - preview[0].y) < 0.001 && document.undo() &&
            document.serialize() == before && document.redo(), "Slab move/undo/redo did not preserve the document");
        reject_without_change(document, [&] { auto resized = document.rectangle(right); resized.width += 1;
            document.set_rectangle(right, resized); }, "Fixed slab accepted a resized footprint");
        for (const auto& [path, invalid] : { std::pair{ "slab/rise", "up" }, { "slab/x", "65500" },
            { "slab/y", "-65536" }, { "material", "pencil" }, { "material", "" },
            { "collision_type", "STRUCTURE_PAINTABLE" } })
            reject_without_change(document, [&] { document.set_property(right, path, invalid); },
                "Slab accepted unsupported contract or lost transactional state");

        auto extended = parse(document.serialize());
        auto& value = extended["collision_objects"][static_cast<rapidjson::SizeType>(right.index)];
        value.AddMember("future_object", "preserve", extended.GetAllocator());
        value["slab"].AddMember("future_slab", "preserve", extended.GetAllocator());
        const auto metadata = scratch / "slab-metadata.json";
        write(metadata, encode(extended)); document.load(metadata);
        document.set_property(right, "slab/rise", "left");
        const Entry copy = document.duplicate(right);
        const auto saved = scratch / "slab-roundtrip.json"; document.save(saved);
        LevelDocument reopened(repo / "game/content/tuning.json"); reopened.load(saved);
        const auto roundtrip = parse(reopened.serialize());
        for (const Entry& entry : { right, copy })
        {
            const auto& kept = roundtrip["collision_objects"][static_cast<rapidjson::SizeType>(entry.index)];
            require(kept["future_object"] == value["future_object"] && kept["slab"]["future_slab"] == value["slab"]["future_slab"] &&
                std::string(kept["slab"]["rise"].GetString()) == "left", "Slab edits/copy/save lost unknown metadata");
        }
        require(reopened.validate().empty() && reopened.slab_guide(copy).has_value(), "Saved slab failed to reopen");
        const Rect original_bounds = reopened.rectangle(right), copy_bounds = reopened.rectangle(copy);
        require(copy_bounds.y > original_bounds.y + original_bounds.height,
            "Left slab duplicate still overlaps its original normal-thick body");
        reopened.set_property(left, "slab/rise", "right");
        const Entry right_copy = reopened.duplicate(left);
        require(reopened.rectangle(right_copy).y > reopened.rectangle(left).y + reopened.rectangle(left).height,
            "Mirrored slab duplicate still overlaps its original body");

        LevelDocument pairs;
        pairs.add_object("FixedSlabRight", { 1240, 880, 1, 1 });
        reject_without_change(pairs, [&] { pairs.add_object("FixedSlabRight", { 1244, 880, 1, 1 }); },
            "Editor admitted intersecting fixed slabs");
        // Their AABBs overlap, but a 40-unit vertical offset separates the
        // actual 24-normal-thick bodies. No AABB shortcut may reject this pair.
        const Entry separate = pairs.add_object("FixedSlabRight", { 1240, 920, 1, 1 });
        require(pairs.validate().empty(), "Editor rejected separated slabs with overlapping AABBs");
        reject_without_change(pairs, [&] { pairs.set_property(separate, "slab/y", "880"); },
            "A property edit intersected fixed slabs without preserving state");

        rapidjson::SchemaDocument schema(parse(contents(repo / "game/level.schema.json")));
        rapidjson::SchemaValidator validator(schema);
        require(roundtrip.Accept(validator), "JSON schema refused a saved fixed slab");
        for (const char* unsupported : { "motion", "drift", "rotation", "rectangle", "rectangle_tlbr", "triangle" })
        {
            auto invalid = parse(reopened.serialize());
            auto& object = invalid["collision_objects"][static_cast<rapidjson::SizeType>(right.index)];
            object.AddMember(rapidjson::Value(unsupported, invalid.GetAllocator()), rapidjson::Value(0), invalid.GetAllocator());
            const auto path = scratch / (std::string("slab-invalid-") + unsupported + ".json");
            write(path, encode(invalid));
            reject_without_change(reopened, [&] { reopened.load(path); }, "Slab load accepted unsupported geometry/motion");
            validator.Reset(); require(!invalid.Accept(validator), "Schema accepted unsupported slab geometry/motion");
        }

        LevelDocument score;
        const Entry scored = score.add_object("FixedSlabRight", { 1240, 880, 1, 1 });
        const auto all = faces_for(score, scored);
        const double expected = 2 * (std::hypot(160.0, 80.0) + 24) * 12;
        require(all.size() == 4 && std::abs(scoring_area(all) - expected) < 0.01 &&
            std::abs(scoring_area(all, score.rectangle(scored)) - scoring_area(all)) < 0.01 &&
            scoring_area(all, Rect{ 1240, 800, 5, 5 }) == 0,
            "Slab scoring lost local face lengths, region clipping or used its empty AABB corner");
        const Entry receiver = score.add_object("StructurePaintable", { 1400, 800, 540, 80 });
        const auto slab_faces = faces_for(score, scored);
        require(std::none_of(slab_faces.begin(), slab_faces.end(), [](const ScoringFace& face)
            { return !face.along_x && !face.leading; }),
            "Buried slab normal cap still scores");
        double receiver_left = 0;
        for (const auto& face : faces_for(score, receiver))
            if (!face.along_x && face.leading) { receiver_left += face.strip.width * face.strip.height; }
        require(std::abs(receiver_left - (80 - 24 * std::sqrt(1.25)) * 12) < 0.05,
            "Reciprocal receiver clipping used slab AABB instead of the actual diagonal solid");
        for (bool rises_right : { true, false })
        {
            LevelDocument seam;
            const Entry slope = seam.add_object(rises_right ? "FixedSlabRight" : "FixedSlabLeft",
                { 0, rises_right ? 80.0 : 0.0, 1, 1 });
            const auto represented = *seam.slab_guide(slope);
            const Entry cover = seam.add_object("Structure", { rises_right ? 160.0 : -160.0, 0, 160, 80 });
            const auto cap_area = [&](const LevelDocument& current)
            {
                double area = 0;
                for (const auto& face : faces_for(current, slope))
                    if (!face.along_x && face.leading == !rises_right)
                        area += face.strip.width * face.strip.height;
                return area;
            };
            require(cap_area(seam) == 0.0, "OBB reconstruction invented a scored strip at an authored slab join");
            // Four-ULP correction of the represented slab must never round a
            // distinct authored obstacle onto that join. This actual gap is
            // smaller than the checked reconstruction bound at world X=160.
            constexpr float gap = 0.00004f;
            seam.set_rectangle(cover, { rises_right ? 160.0 : -160.0, gap, 160, 80 });
            const double exposed = cap_area(seam);
            require(exposed > 0 && std::abs(exposed - 12.0 * gap * std::sqrt(5.0) / 2.0) < 0.0000001,
                "Canonical occlusion erased a real authored small gap or changed its local normal area");
            const auto unchanged = *seam.slab_guide(slope);
            for (std::size_t index = 0; index < represented.size(); ++index)
                require(represented[index].x == unchanged[index].x && represented[index].y == unchanged[index].y,
                    "Paint adjacency correction changed the represented slab guide");
        }
        // A mixed saved document exercises mover-envelope parsing of the new
        // solid even though the admitted mover is deliberately far away.
        document.add_object("MovingPaintable", { 1800, 600, 320, 20 });
        document.save(scratch / "slab-and-mover.json");
        std::cout << "PASS fixed-slab palette, mirrored normal geometry, immutable footprint, transactional metadata/history/save, "
            "schema and actual polygon scoring\n";
    }
}
