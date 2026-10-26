// Per-field validation checks: ending audio, the Random setting, materials
// and drift.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <rapidjson/schema.h>

#include <iostream>
#include <map>
#include <string>

namespace level_editor_acceptance
{
    void check_ending_audio(const fs::path& repo)
    {
        LevelDocument document;
        document.load(repo / "game/content/levels/close_quarters.json");
        const Entry level = entry(document, EntryKind::level);
        for (const char* invalid : { "-0.01", "1.01", "1e100" })
        {
            reject_without_change(document, [&]
            {
                document.set_property(level, "/sound/end_sting_volume", invalid);
            }, "The editor accepted an invalid ending-sting volume");
        }
        reject_without_change(document, [&]
        {
            document.set_property(level, "/sound/end_sting_name", "");
        }, "The editor accepted an empty ending-sting name");
        for (const char* valid : { "0", "1" })
        {
            document.set_property(level, "/sound/end_sting_volume", valid);
            require(document.validate().empty(), "The editor rejected an ending-sting gain boundary");
        }
        for (const char* invalid : { "-0.01", "1.01", "1e100" })
        {
            reject_without_change(document, [&]
            {
                document.set_property(level, "/sound/music_volume", invalid);
            }, "The editor accepted an invalid music volume");
        }
        for (const char* valid : { "0", "1" })
        {
            document.set_property(level, "/sound/music_volume", valid);
            require(document.validate().empty(), "The editor rejected a music gain boundary");
        }
        std::cout << "PASS ending-sting and music editor validation and gain boundaries\n";
    }

    void check_random_setting(const fs::path& scratch)
    {
        LevelDocument document;
        const fs::path path = scratch / "random-setting.json";
        document.save(path);
        const Entry level = entry(document, EntryKind::level);
        const std::string original = document.serialize();
        require(!parse(original).HasMember("include_in_random"), "Fixture must omit Random setting");
        std::size_t count = 0;
        for (const Property& property : document.properties(level))
        {
            if (property.path == "include_in_random")
            {
                ++count;
                require(property.boolean && property.value == "true", "Missing Random setting must show true");
            }
        }
        require(count == 1 && document.serialize() == original && !document.dirty() &&
            !document.can_undo(), "Inspecting the default must not edit the document");
        reject_without_change(document, [&] { document.set_property(level, "include_in_random", "0"); },
            "Invalid optional boolean was accepted");
        reject_without_change(document, [&] { document.set_property(level, "unknown_optional", "false"); },
            "Inspector inserted an unknown missing field");
        document.set_property(level, "include_in_random", "false");
        require(!parse(document.serialize())["include_in_random"].GetBool() && document.dirty(),
            "Applying false must add the Random setting");
        require(document.undo() && document.serialize() == original && !document.dirty(),
            "Undo must restore omission and saved state");
        require(document.redo() && !parse(document.serialize())["include_in_random"].GetBool(),
            "Redo must restore explicit false");
        document.save(path);
        LevelDocument reopened;
        reopened.load(path);
        require(!parse(reopened.serialize())["include_in_random"].GetBool(), "Save/reopen lost false");
        count = 0;
        for (const Property& property : reopened.properties(level))
            if (property.path == "include_in_random")
            {
                ++count;
                require(property.boolean && property.value == "false", "Existing false must show false");
            }
        require(count == 1, "Existing Random setting must appear once");
        reopened.set_property(level, "/include_in_random", "true");
        require(parse(reopened.serialize())["include_in_random"].GetBool(), "Existing setting did not toggle");
        reject_without_change(reopened, [&] { reopened.set_property(level, "include_in_random", "yes"); },
            "Invalid existing boolean was accepted");
        std::string malformed = original;
        malformed.insert(malformed.find('{') + 1, "\n\"include_in_random\": \"false\",");
        const fs::path invalid = scratch / "invalid-random-setting.json";
        write(invalid, malformed);
        reject_without_change(reopened, [&] { reopened.load(invalid); }, "Non-boolean Random setting loaded");
        std::cout << "PASS optional Random setting, validation, history and persistence\n";
    }

    void check_materials(const fs::path& scratch)
    {
        LevelDocument document;
        const auto material_of = [&](const Entry& item) -> std::string
        {
            const rapidjson::Document json = parse(document.serialize());
            const rapidjson::Value& object = json[item.kind == EntryKind::collision ?
                "collision_objects" : "non_collision_objects"][static_cast<rapidjson::SizeType>(item.index)];
            return object.HasMember("material") ? object["material"].GetString() : "";
        };
        const std::map<std::string, std::string> defaults = {
            { "Structure", "ground" }, { "StructureJumpThrough", "ruler" },
            { "StructurePaintable", "pencil" }, { "StructureRampRight", "set_square" },
            { "StructureRampLeft", "set_square" }, { "Visual", "" } };
        std::map<std::string, Entry> added;
        for (const auto& [type, material] : defaults)
        {
            added[type] = document.add_object(type, { 100, 200, 300, 150 });
            require(material_of(added[type]) == material, type + " did not get its default material");
        }
        const Entry structure = added["Structure"];
        reject_without_change(document, [&] { document.set_property(structure, "/material", "chalk"); },
            "Unknown material was accepted");
        reject_without_change(document, [&] { document.set_property(structure, "/material", "pencil"); },
            "Paintable-only material was accepted on a solid structure");
        reject_without_change(document, [&] { document.set_property(added["StructureRampLeft"], "/material", "ground"); },
            "Ground was accepted on a ramp");
        reject_without_change(document, [&] { document.set_property(added["StructurePaintable"], "/material", "ruler"); },
            "Ruler was accepted on a paintable structure");
        reject_without_change(document, [&] { document.set_property(structure, "/collision_type", "STRUCTURE_JUMP_THROUGH"); },
            "Changing collision under a ground material was accepted");
        reject_without_change(document, [&] { document.set_property(added["Visual"], "material", "ground"); },
            "Inspector inserted a material on a decoration");
        reject_without_change(document, [&] { document.set_property(structure, "unknown_optional", "ground"); },
            "Inspector inserted an unknown missing field on a collision object");
        for (const char* material : { "pencil", "eraser", "paint_tin", "ground" })
        {
            document.set_property(added["StructurePaintable"], "/material", material);
            require(material_of(added["StructurePaintable"]) == material, "Allowed paintable material was refused");
        }

        // Clearing returns the legacy draw; the inspector then offers the field back.
        const std::string with_material = document.serialize();
        document.set_property(structure, "material", "");
        require(material_of(structure).empty(), "Clearing material did not remove the field");
        std::size_t count = 0;
        for (const Property& property : document.properties(structure))
            if (property.path == "material")
            {
                ++count;
                require(property.value.empty() && property.label.find("ground") != std::string::npos &&
                    property.label.find("pencil") == std::string::npos,
                    "Missing material must show empty with only its collision's choices");
            }
        require(count == 1, "A collision object without material must offer it once");
        for (const Property& property : document.properties(added["Visual"]))
            require(property.path != "material", "A decoration must not offer a material");
        reject_without_change(document, [&] { document.set_property(structure, "material", "ruler"); },
            "Inspector added a mismatched missing material");
        document.set_property(structure, "material", "ground");
        require(document.serialize() == with_material, "Adding material back did not restore it");
        require(document.undo() && material_of(structure).empty(), "Undo must restore the omission");
        document.set_property(structure, "material", "ground");

        const fs::path path = scratch / "materials.json";
        document.save(path);
        LevelDocument reopened;
        reopened.load(path);
        require(parse(reopened.serialize()) == parse(document.serialize()), "Materials did not survive saving");
        const std::string saved = contents(path);
        const std::string ground = "\"material\": \"ground\"";
        const std::size_t at = saved.find(ground);
        require(at != std::string::npos, "Saved fixture lacks a ground material");
        for (const char* bad : { "\"material\": \"chalk\"", "\"material\": \"ruler\"", "\"material\": 3" })
        {
            std::string changed = saved;
            changed.replace(at, ground.size(), bad);
            const fs::path invalid = scratch / "invalid-material.json";
            write(invalid, changed);
            reject_without_change(reopened, [&] { reopened.load(invalid); }, "Invalid material loaded");
        }
        // Paintability does not require solid player collision. Preserve the
        // explicit top-only role while editing and round-tripping a thin ruler.
        rapidjson::Document scored = parse(document.serialize());
        const Entry support = added["StructurePaintable"];
        rapidjson::Value& ruler = scored["collision_objects"][static_cast<rapidjson::SizeType>(support.index)];
        ruler["collision_type"].SetString("STRUCTURE_JUMP_THROUGH");
        ruler["material"].SetString("ruler");
        ruler["rectangle"]["height"].SetDouble(10);
        for (const char* face : { "left", "bottom", "right" })
        { ruler["paintable_faces"][face].SetBool(false); }
        ruler["paintable_faces"]["top"].SetBool(true);
        const fs::path scored_path = scratch / "paintable-jump-through.json";
        write(scored_path, encode(scored));
        LevelDocument scored_document;
        scored_document.load(scored_path);
        require(scored_document.validate().empty(), "Editor rejected a paintable jump-through ruler");
        scored_document.set_rectangle(support, { 120, 260, 320, 10 });
        scored_document.set_property(support, "/paintable_faces/top", "false");
        require(scored_document.undo(), "Paintable ruler face edit lacked undo");
        scored_document.save(scored_path);
        LevelDocument scored_reopened;
        scored_reopened.load(scored_path);
        const rapidjson::Document scored_json = parse(scored_reopened.serialize());
        const rapidjson::Value& retained = scored_json["collision_objects"][static_cast<rapidjson::SizeType>(support.index)];
        require(parse(scored_document.serialize()) == scored_json &&
            std::string(retained["type"].GetString()) == "StructurePaintable" &&
            std::string(retained["collision_type"].GetString()) == "STRUCTURE_JUMP_THROUGH" &&
            retained["paintable_faces"]["top"].GetBool() && !retained["paintable_faces"]["bottom"].GetBool() &&
            equal(scored_reopened.rectangle(support), { 120, 260, 320, 10 }),
            "Editing or saving a paintable ruler changed collision, faces or physical support depth");
        reject_without_change(scored_reopened,
            [&] { scored_reopened.set_property(support, "/material", "ground"); },
            "Paintable jump-through accepted a solid ground material");
        std::cout << "PASS optional materials: defaults, allowed types, inspector insert/clear and load refusal\n";
        std::cout << "PASS paintable ruler: independent jump-through tag, top-only face, edit/undo and saved round trip\n";
    }

    void check_drift(const fs::path& repo, const fs::path& scratch)
    {
        LevelDocument base;
        // Runtime motion is separate from decorative drift. Refuse opening a
        // moving stage until the editor can show and edit its travel envelope.
        rapidjson::Document moving = parse(contents(repo / "editor/template.json"));
        rapidjson::Document motion;
        motion.Parse("{\"type\":\"horizontal\",\"travel_x\":120,\"period\":6.4,\"phase\":0.25}");
        rapidjson::Value motion_value;
        motion_value.CopyFrom(motion, moving.GetAllocator());
        moving["collision_objects"][3].AddMember("motion", motion_value, moving.GetAllocator());
        const fs::path moving_path = scratch / "unsupported-runtime-motion.json";
        const std::string moving_source = encode(moving);
        write(moving_path, moving_source);
        std::string motion_diagnostic;
        reject_without_change(base, [&]
        {
            try { base.load(moving_path); }
            catch (const std::exception& error) { motion_diagnostic = error.what(); throw; }
        }, "Editor accepted motion without envelope-aware authoring support");
        require(motion_diagnostic.find("motion") != std::string::npos &&
            motion_diagnostic.find("cannot yet be edited safely") != std::string::npos &&
            contents(moving_path) == moving_source,
            "Editor motion refusal lost its explanation or changed the source");
        rapidjson::Document valid = parse(base.serialize());
        rapidjson::Document drift;
        drift.Parse("{\"distance\":32,\"period\":60,\"phase\":0.25,\"author_note\":\"retain\"}");
        rapidjson::Value authored;
        authored.CopyFrom(drift, valid.GetAllocator());
        valid["non_collision_objects"][0].AddMember("drift", authored, valid.GetAllocator());
        const rapidjson::Document schema_json = parse(contents(repo / "docs" / "level.schema.json"));
        const rapidjson::SchemaDocument schema(schema_json);
        const auto schema_accepts = [&](const rapidjson::Document& value)
        {
            rapidjson::SchemaValidator validator(schema);
            return value.Accept(validator);
        };
        require(schema_accepts(valid), "The schema refused valid decorative drift");

        const fs::path path = scratch / "drift.json";
        write(path, encode(valid));
        LevelDocument document;
        document.load(path);
        const Entry visual = entry(document, EntryKind::decoration);
        std::size_t drift_fields = 0;
        for (const Property& property : document.properties(visual))
        {
            if (property.path == "drift/distance" || property.path == "drift/period" ||
                property.path == "drift/phase")
            {
                require(property.numeric && !property.boolean,
                    "The drift inspector field must be numeric");
                ++drift_fields;
            }
        }
        require(drift_fields == 3, "The inspector omitted a drift field");
        document.set_property(visual, "drift/distance", "48");
        require(parse(document.serialize())["non_collision_objects"][0]["drift"]["distance"] == 48,
            "Editing drift changed the wrong field");
        require(document.undo() && parse(document.serialize()) == valid,
            "Undo did not restore the original drift");
        require(document.redo(), "A drift edit did not enter redo history");
        document.save(path);
        LevelDocument reopened;
        reopened.load(path);
        require(parse(reopened.serialize()) == parse(document.serialize()),
            "Save/reopen changed drift or its unknown metadata");
        for (const auto& [field, value] : std::map<std::string, std::string>{
            { "distance", "0" }, { "period", "0.5" }, { "phase", "1" } })
        {
            reject_without_change(document, [&] { document.set_property(visual, "drift/" + field, value); },
                "An invalid drift edit changed the document");
        }

        const auto variant = [&](const char* name, bool expected, const auto& change)
        {
            rapidjson::Document changed = parse(encode(valid));
            change(changed);
            require(schema_accepts(changed) == expected,
                std::string("The schema disagreed about drift fixture ") + name);
            const fs::path fixture = scratch / (std::string("drift-") + name + ".json");
            write(fixture, encode(changed));
            if (expected)
            {
                LevelDocument accepted;
                accepted.load(fixture);
                require(parse(accepted.serialize()) == changed,
                    std::string("The editor changed drift fixture ") + name);
            }
            else
            {
                reject_without_change(reopened, [&] { reopened.load(fixture); },
                    "The editor accepted invalid drift");
            }
        };
        variant("absent", true, [](auto& d) { d["non_collision_objects"][0].RemoveMember("drift"); });
        variant("minimum", true, [](auto& d)
        {
            auto& value = d["non_collision_objects"][0]["drift"];
            value["distance"].SetDouble(0.25); value["period"].SetInt(1); value["phase"].SetInt(0);
        });
        variant("maximum", true, [](auto& d)
        {
            auto& value = d["non_collision_objects"][0]["drift"];
            value["distance"].SetInt(256); value["period"].SetInt(3600); value["phase"].SetDouble(0.999);
        });
        variant("not-object", false, [](auto& d) { d["non_collision_objects"][0]["drift"].SetInt(2); });
        variant("missing-distance", false, [](auto& d) { d["non_collision_objects"][0]["drift"].RemoveMember("distance"); });
        variant("missing-period", false, [](auto& d) { d["non_collision_objects"][0]["drift"].RemoveMember("period"); });
        variant("missing-phase", false, [](auto& d) { d["non_collision_objects"][0]["drift"].RemoveMember("phase"); });
        variant("string-distance", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["distance"].SetString("32"); });
        variant("boolean-period", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["period"].SetBool(true); });
        variant("null-phase", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["phase"].SetNull(); });
        variant("zero-distance", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["distance"].SetInt(0); });
        variant("negative-distance", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["distance"].SetInt(-1); });
        variant("large-distance", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["distance"].SetInt(257); });
        variant("short-period", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["period"].SetDouble(0.999); });
        variant("long-period", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["period"].SetInt(3601); });
        variant("negative-phase", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["phase"].SetDouble(-0.001); });
        variant("complete-phase", false, [](auto& d) { d["non_collision_objects"][0]["drift"]["phase"].SetInt(1); });
        variant("collision", false, [](auto& d)
        {
            rapidjson::Value value;
            value.CopyFrom(d["non_collision_objects"][0]["drift"], d.GetAllocator());
            d["collision_objects"][0].AddMember("drift", value, d.GetAllocator());
        });
        // JSON Schema checks authored numbers; game/editor also check the
        // values after float conversion, as they do for rectangle sizes.
        for (const auto& [field, value] : std::map<std::string, double>{
            { "distance", 1.0e-100 }, { "phase", 0.999999999 } })
        {
            rapidjson::Document changed = parse(encode(valid));
            changed["non_collision_objects"][0]["drift"][field.c_str()].SetDouble(value);
            require(schema_accepts(changed), "A float-conversion fixture failed structurally");
            const fs::path fixture = scratch / "drift-float-conversion.json";
            write(fixture, encode(changed));
            reject_without_change(reopened, [&] { reopened.load(fixture); },
                "The editor accepted drift that leaves its range after float conversion");
        }
        std::cout << "PASS optional drift: schema/editor agreement, numeric limits and type/missing-field refusals, inspector history and metadata-preserving save/reopen\n";
    }
}
