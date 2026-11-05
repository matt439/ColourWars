#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/blockout_aids.h"
#include <iostream>

namespace level_editor_acceptance
{
    void check_motion(const fs::path& repo, const fs::path& scratch)
    {
        const auto tuning = repo / "game/content/tuning.json";
        const std::string tuning_source = contents(tuning);
        const auto size_json = parse(tuning_source);
        const double body_width = size_json["player"]["size"]["x"].GetDouble();
        const double body_height = size_json["player"]["size"]["y"].GetDouble();
        LevelDocument document(tuning);
        const Entry moving = document.add_object("MovingPaintable", { 600, 600, 320, 20 });
        const Entry one_way = document.add_object("MovingJumpThrough", { 1800, 700, 320, 20 });
        const auto original = parse(document.serialize());
        require(original["collision_objects"][static_cast<rapidjson::SizeType>(moving.index)]["type"] == "StructurePaintable" &&
            std::string(original["collision_objects"][static_cast<rapidjson::SizeType>(one_way.index)]["collision_type"].GetString()) ==
                "STRUCTURE_JUMP_THROUGH" &&
            std::string(original["collision_objects"][static_cast<rapidjson::SizeType>(one_way.index)]["material"].GetString()) == "ruler",
            "Mover palette changed the runtime paintable/collision/material contract");
        for (const char* path : { "motion/travel_x", "motion/period", "motion/phase" })
        {
            bool found = false;
            for (const Property& property : document.properties(moving))
            { found |= property.path == path && property.numeric && !property.boolean; }
            require(found, std::string("Missing numeric mover inspector field: ") + path);
        }
        document.set_property(moving, "motion/phase", "0.25");
        document.set_rectangle(moving, { 650, 620, 360, 24 });
        const auto guide = *document.motion_guide(moving);
        require(equal(guide.anchor, { 650, 620, 360, 24 }) && equal(guide.initial_pose, { 710, 620, 360, 24 }) &&
            equal(guide.travel_envelope, { 650, 620, 480, 24 }) &&
            guide.standing_envelope.x < 650 - body_width &&
            guide.standing_envelope.y < 620 - body_height &&
            guide.standing_envelope.width > 480 + 2 * body_width,
            "Motion guides omitted authored phase, full travel or source-tuned padded standing corridor");
        require(!document.motion_guide(Entry{ EntryKind::level, 0, "" }), "Static metadata produced a motion guide");
        const auto preview = *document.motion_guide(moving, Rect{ 680, 640, 400, 30 });
        require(equal(preview.anchor, { 680, 640, 400, 30 }) && equal(preview.initial_pose, { 740, 640, 400, 30 }) &&
            equal(document.rectangle(moving), { 650, 620, 360, 24 }), "Provisional anchor guide edited the document");
        const std::string resized = document.serialize();
        document.set_rectangle(moving, document.rectangle(moving));
        require(document.serialize() == resized, "No-op anchor edit changed numeric spelling/history");
        require(document.undo() && equal(document.rectangle(moving), { 600, 600, 320, 20 }) &&
            document.redo() && document.serialize() == resized, "Mover resize did not undo/redo exactly");

        // Preserve nested root/object/motion metadata through actual edits and
        // TLBR geometry. An ignored malformed alternative remains ignored too.
        auto extended = parse(document.serialize());
        extended.AddMember("future_root", "preserve", extended.GetAllocator());
        auto& value = extended["collision_objects"][static_cast<rapidjson::SizeType>(moving.index)];
        value.AddMember("future_object", "preserve", extended.GetAllocator());
        rapidjson::Value metadata(rapidjson::kObjectType);
        metadata.AddMember("nested", "preserve", extended.GetAllocator());
        value["motion"].AddMember("future_motion", metadata, extended.GetAllocator());
        value.AddMember("rectangle_tlbr", "ignored", extended.GetAllocator());
        const auto extended_path = scratch / "motion-metadata.json";
        write(extended_path, encode(extended));
        document.load(extended_path);
        document.set_property(moving, "motion/period", "8");
        document.set_rectangle(moving, { 675, 625, 360, 24 });
        auto after = parse(document.serialize());
        const auto& retained = after["collision_objects"][static_cast<rapidjson::SizeType>(moving.index)];
        require(after["future_root"] == extended["future_root"] &&
            retained["future_object"] == value["future_object"] && retained["motion"]["future_motion"] == value["motion"]["future_motion"] &&
            retained["rectangle_tlbr"] == value["rectangle_tlbr"] &&
            retained["motion"]["travel_x"].GetDouble() == 120 && retained["motion"]["phase"].GetDouble() == 0.25,
            "Anchor move/resize rewrote unrelated metadata, ignored geometry, travel or phase");
        rapidjson::Value tlbr(rapidjson::kObjectType);
        tlbr.AddMember("left", 675, after.GetAllocator()); tlbr.AddMember("top", 625, after.GetAllocator());
        tlbr.AddMember("right", 1035, after.GetAllocator()); tlbr.AddMember("bottom", 649, after.GetAllocator());
        auto& alternate = after["collision_objects"][static_cast<rapidjson::SizeType>(moving.index)];
        alternate.RemoveMember("rectangle"); alternate["rectangle_tlbr"].Swap(tlbr);
        const auto tlbr_path = scratch / "motion-tlbr.json";
        write(tlbr_path, encode(after)); document.load(tlbr_path);
        document.set_rectangle(moving, { 700, 650, 360, 24 });
        require(!parse(document.serialize())["collision_objects"][static_cast<rapidjson::SizeType>(moving.index)].HasMember("rectangle") &&
            equal(document.motion_guide(moving)->anchor, { 700, 650, 360, 24 }), "TLBR anchor editing changed encoding or runtime float geometry");

        for (const auto& [path, invalid] : {
            std::pair{ "motion/travel_x", "0" }, { "motion/travel_x", "-1" },
            { "motion/travel_x", "1e-40" }, { "motion/travel_x", "1e40" },
            { "motion/period", "0.01" }, { "motion/period", "1e40" },
            { "motion/phase", "-0.1" }, { "motion/phase", "1" }, { "motion/phase", "0.999999999999" },
            { "motion/type", "vertical" }, { "collision_type", "STRUCTURE" } })
        {
            reject_without_change(document, [&] { document.set_property(moving, path, invalid); },
                "Editor accepted unsupported/malformed motion or changed transactional state");
        }
        reject_without_change(document, [&] { document.set_rectangle(moving, { -190, 650, 360, 24 }); },
            "Editor accepted standing corridor outside Scene bounds");
        reject_without_change(document, [&] { document.set_rectangle(moving, { 700, 1190, 360, 24 }); },
            "Editor accepted platform envelope inside floor geometry");
        reject_without_change(document, [&] { document.add_object("Structure", { 750, 560, 40, 40 }); },
            "A static edit closed an admitted standing corridor");
        reject_without_change(document, [&] { document.set_rectangle(Entry{ EntryKind::bounds, 0, "" },
            { -200, 600, 3400, 1000 }); }, "A bounds edit clipped an admitted standing corridor");
        const Entry copy = document.duplicate(moving);
        const auto source_guide = *document.motion_guide(moving);
        const auto copy_guide = *document.motion_guide(copy);
        require(copy_guide.standing_envelope.y >= source_guide.travel_envelope.y + source_guide.travel_envelope.height &&
            copy_guide.anchor.x == source_guide.anchor.x && copy_guide.travel_envelope.width == source_guide.travel_envelope.width,
            "Mover duplicate overlaps its original complete path/corridor or silently scales travel");
        require(document.undo() && document.redo(), "Mover duplicate lacked undo/redo");
        document.set_rectangle(moving, { 700, 1180, 360, 20 });
        reject_without_change(document, [&] { document.duplicate(moving); },
            "Blocked mover duplicate was published before complete validation");
        const BlockoutTuning scoring = load_blockout_tuning(tuning, repo / "editor/movement-baseline.json");
        const double expected = 3000 * scoring.paint_thickness +
            2 * (360 + 20 + 320 + 20 + 360 + 24) * scoring.paint_thickness;
        require(scoring_area(scoring_faces(document, scoring)) == expected,
            "Moving anchor buried static floor paint, counted its travel sweep or lost local faces");
        const double before_phase = scoring_area(scoring_faces(document, scoring));
        document.set_property(moving, "motion/phase", "0.75");
        require(scoring_area(scoring_faces(document, scoring)) == before_phase,
            "Initial phase changed total locally attached scoring area");

        const auto saved_path = scratch / "motion-authored.json";
        document.save(saved_path);
        LevelDocument reopened(tuning); reopened.load(saved_path);
        require(parse(reopened.serialize()) == parse(document.serialize()) && !reopened.dirty() &&
            contents(tuning) == tuning_source, "Authored motion lost saved fields or touched source tuning");

        // A different source size changes the guide; it is never a guessed
        // constant or the measurements file's stale baseline.
        auto changed_tuning = parse(tuning_source);
        changed_tuning["player"]["size"]["x"].SetDouble(body_width + 18);
        changed_tuning["player"]["size"]["y"].SetDouble(body_height + 15);
        const auto changed_path = scratch / "motion-source-size.json";
        write(changed_path, encode(changed_tuning));
        LevelDocument changed(changed_path);
        changed.load(extended_path);
        const auto changed_guide = *changed.motion_guide(moving);
        require(changed_guide.standing_envelope.width > guide.standing_envelope.width + 35 &&
            changed_guide.standing_envelope.y < guide.standing_envelope.y - 14,
            "Standing guides ignored changed source player dimensions");

        // Crossing a float binade changes represented world endpoint spacing.
        // Scored area belongs to the anchor and must not be remeasured there.
        LevelDocument binade_source;
        auto binade_json = parse(binade_source.serialize());
        binade_json["level_bounds"]["out_of_bounds"]["width"].SetDouble(140200);
        for (rapidjson::SizeType index = 0; index < 4; ++index)
        { binade_json["spawn_locations"]["team_b"][index]["x"].SetDouble(130000 + index * 100); }
        rapidjson::Value fractional;
        fractional.CopyFrom(extended["collision_objects"][static_cast<rapidjson::SizeType>(moving.index)], binade_json.GetAllocator());
        fractional["rectangle"]["x"].SetDouble(65535.99);
        fractional["rectangle"]["y"].SetDouble(800);
        fractional["rectangle"]["width"].SetDouble(320);
        fractional["rectangle"]["height"].SetDouble(20);
        fractional["motion"]["phase"].SetDouble(0);
        binade_json["collision_objects"].PushBack(fractional, binade_json.GetAllocator());
        const auto binade_path = scratch / "motion-binade.json";
        write(binade_path, encode(binade_json));
        LevelDocument binade(tuning); binade.load(binade_path);
        const Entry fractional_entry{ EntryKind::collision, binade_json["collision_objects"].Size() - 1, "" };
        const auto local_faces = scoring_faces(binade, scoring);
        const double local_area = scoring_area(local_faces);
        binade.set_property(fractional_entry, "motion/phase", "0.25");
        const auto shifted_faces = scoring_faces(binade, scoring);
        require(scoring_area(shifted_faces) == local_area && local_faces.size() == shifted_faces.size(),
            "Float-binade phase translation changed locally attached editor scoring area");
        bool moved_face = false;
        for (size_t index = 0; index < local_faces.size(); ++index)
        {
            if (local_faces[index].entry.index != fractional_entry.index) { continue; }
            require(local_faces[index].strip.width == shifted_faces[index].strip.width &&
                local_faces[index].strip.height == shifted_faces[index].strip.height &&
                shifted_faces[index].strip.x == static_cast<float>(local_faces[index].strip.x) + 60.0f,
                "Moving scoring guide recomputed local dimensions or failed to translate its display");
            moved_face = true;
        }
        require(moved_face, "Float-binade control did not inspect an actual moving scoring face");
        std::cout << "PASS mover palette/phase/travel/standing guides, transactional anchor/duplicate/history, metadata/TLBR and saved local scoring\n";
        std::cout << "PASS fractional float-binade phases retain local scoring dimensions and translate guide locations\n";
        check_lift_motion(repo, scratch);
    }

    // A vertical lift (ROAD#28): its own palette entry, a top anchor and
    // travel_y, the clear room beneath its lowest pose, solid and cycle only,
    // duplicates past that room and a mirror that reflects only the anchor.
    void check_lift_motion(const fs::path& repo, const fs::path& scratch)
    {
        const auto tuning = repo / "game/content/tuning.json";
        const auto size_json = parse(contents(tuning));
        const double body_width = size_json["player"]["size"]["x"].GetDouble();
        const double body_height = size_json["player"]["size"]["y"].GetDouble();
        LevelDocument document(tuning);
        const Entry lift = document.add_object("VerticalLift", { 600, 600, 120, 20 });
        const auto saved = parse(document.serialize());
        const auto& record = saved["collision_objects"][static_cast<rapidjson::SizeType>(lift.index)];
        require(std::string(record["type"].GetString()) == "StructurePaintable" &&
            std::string(record["collision_type"].GetString()) == "STRUCTURE_PAINTABLE" &&
            std::string(record["motion"]["type"].GetString()) == "vertical" &&
            record["motion"]["travel_y"].GetDouble() == 320 && !record["motion"].HasMember("travel_x") &&
            record["motion"]["period"].GetDouble() == 4 && record["motion"]["phase"].GetDouble() == 0,
            "The lift palette entry did not write a solid cycling one-storey lift");
        bool travel_field = false;
        for (const Property& property : document.properties(lift))
        { travel_field |= property.path == "motion/travel_y" && property.numeric && property.label.find("down") != std::string::npos; }
        require(travel_field, "The lift inspector lacks its labelled numeric travel_y field");

        document.set_property(lift, "motion/phase", "0.25");
        const auto guide = *document.motion_guide(lift);
        require(equal(guide.anchor, { 600, 600, 120, 20 }) && equal(guide.initial_pose, { 600, 760, 120, 20 }) &&
            equal(guide.travel_envelope, { 600, 600, 120, 340 }) &&
            guide.standing_envelope.y < 600 - body_height && guide.standing_envelope.x < 600 - body_width &&
            guide.clearance_envelope && guide.clearance_envelope->y < 600 - body_height &&
            guide.clearance_envelope->y + guide.clearance_envelope->height > 940 + body_height &&
            guide.clearance_envelope->y + guide.clearance_envelope->height < 940 + body_height + 0.01 &&
            guide.clearance_envelope->x < 600 - body_width && guide.clearance_envelope->x > 600 - body_width - 0.01 &&
            guide.clearance_envelope->width > 120 + 2 * body_width,
            "Lift guides omitted its phase, shaft, corridor above or clearance beneath");
        const Entry mover = document.add_object("MovingPaintable", { 1600, 500, 320, 20 });
        require(!document.motion_guide(mover)->clearance_envelope, "A horizontal mover drew a lift's clearance");
        // A ledge within a body of the shaft is refused; a body and a little more is not.
        reject_without_change(document, [&] { document.add_object("Structure", { 730, 800, 100, 40 }); },
            "Editor accepted geometry within a body of a lift's shaft");

        // The floor is at 1200: a lowest bottom past 1080 leaves less than a body.
        reject_without_change(document, [&] { document.set_rectangle(lift, { 600, 800, 120, 20 }); },
            "Editor accepted a lift with less than a body height beneath its lowest pose");
        for (const auto& [path, invalid] : {
            std::pair{ "motion/travel_y", "0" }, { "motion/travel_y", "-1" }, { "motion/travel_y", "1e40" },
            // A paint drive is a lift's own (drive_checks); a heading on its cycle is not.
            { "motion/team_a_heads", "up" }, { "motion/type", "horizontal" },
            { "collision_type", "STRUCTURE_JUMP_THROUGH" } })
        {
            reject_without_change(document, [&] { document.set_property(lift, path, invalid); },
                "Editor accepted an unsupported lift field or changed transactional state");
        }
        document.set_property(lift, "motion/travel_y", "256");
        require(document.motion_guide(lift)->travel_envelope.height == 276, "A lift's travel edit was not applied");

        // Here a copy cannot clear the clearance above the 1200 floor, so it is refused.
        reject_without_change(document, [&] { document.duplicate(lift); },
            "A lift duplicate that would meet the floor was published");
        // Raised to 150, a copy fits below the source's clearance (its path ends 942).
        document.set_rectangle(lift, { 600, 150, 120, 20 });
        const Entry copy = document.duplicate(lift);
        const auto source = *document.motion_guide(lift);
        const auto duplicate = *document.motion_guide(copy);
        require(duplicate.clearance_envelope->y >= source.clearance_envelope->y + source.clearance_envelope->height &&
            duplicate.anchor.x == source.anchor.x && duplicate.travel_envelope.height == source.travel_envelope.height,
            "A lift duplicate overlaps its original's shaft, corridor or clearance");
        require(document.undo(), "Lift duplicate lacked undo");

        // Mirror: x = 1500 for the template. A lift's anchor reflects; its phase stays.
        const Entry mirrored = document.mirror_copy(lift);
        const auto mirror_document = parse(document.serialize());
        const auto& reflected = mirror_document["collision_objects"][static_cast<rapidjson::SizeType>(mirrored.index)];
        require(reflected["rectangle"]["x"].GetDouble() == 3000 - 600 - 120 &&
            reflected["rectangle"]["y"].GetDouble() == 150 && reflected["motion"]["phase"].GetDouble() == 0.25 &&
            reflected["motion"]["travel_y"].GetDouble() == 256,
            "A lift's mirror reflected its travel or advanced its phase");

        // Blockout scoring faces move with the initial phase on y, keeping their size.
        const BlockoutTuning scoring = load_blockout_tuning(tuning, repo / "editor/movement-baseline.json");
        const auto at_quarter = scoring_faces(document, scoring);
        document.set_property(lift, "motion/phase", "0");
        const auto at_top = scoring_faces(document, scoring);
        require(at_quarter.size() == at_top.size() && scoring_area(at_quarter) == scoring_area(at_top),
            "A lift's phase changed its local scoring faces");
        bool moved = false;
        for (std::size_t index = 0; index < at_top.size(); ++index)
        {
            if (at_top[index].entry.index != lift.index || at_top[index].entry.kind != lift.kind) { continue; }
            require(at_quarter[index].strip.x == at_top[index].strip.x &&
                at_quarter[index].strip.y == static_cast<float>(at_top[index].strip.y) + 128.0f &&
                at_quarter[index].strip.width == at_top[index].strip.width,
                "A lift's scoring guide did not translate on y alone");
            moved = true;
        }
        require(moved, "The lift scoring control inspected no lift face");

        const auto saved_path = scratch / "lift-authored.json";
        document.save(saved_path);
        LevelDocument reopened(tuning); reopened.load(saved_path);
        require(parse(reopened.serialize()) == parse(document.serialize()), "An authored lift did not save and reopen");
        std::cout << "PASS lift palette, travel_y inspector, shaft/corridor/clearance guides and refusals, "
            "duplicate past the clearance, anchor-only mirror, y-translated scoring and saved lifts\n";
    }
}
