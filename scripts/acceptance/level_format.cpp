#include "game/pch.h"
#include "game/level_format.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/content_validation.h"
#include "game/simulation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/assets/sound_bank_loader.h"
#include "editor/content_catalog.h"
#include "scripts/acceptance/support/probe.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <set>
#include <typeinfo>

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    rapidjson::Document read_document(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        require(input.good(), "Cannot read " + path.string());
        const std::string source{ std::istreambuf_iterator<char>(input), {} };
        rapidjson::Document document;
        document.Parse(source.c_str());
        require(!document.HasParseError(), "Cannot parse " + path.string());
        return document;
    }

    rapidjson::Document copy_document(const rapidjson::Document& source)
    {
        rapidjson::Document result;
        result.CopyFrom(source, result.GetAllocator());
        return result;
    }

    std::vector<std::filesystem::path> manifest_levels(const std::filesystem::path& content)
    {
        const auto root = content.lexically_normal();
        const auto manifest = read_document(root / "manifest.json");
        require(manifest.IsObject() && manifest.HasMember("assets") && manifest["assets"].IsArray(),
            "Level inventory needs the actual manifest assets");
        std::vector<std::filesystem::path> result;
        std::set<std::filesystem::path> expected, directories{ root / "levels" };
        for (const auto& asset : manifest["assets"].GetArray())
        {
            if (std::string(asset["kind"].GetString()) != "level") { continue; }
            const auto directory = (root / asset["directory"].GetString()).lexically_normal();
            directories.insert(directory);
            for (const auto& name : asset["names"].GetArray())
            {
                const auto relative = (std::filesystem::path(asset["directory"].GetString()) /
                    (std::string(name.GetString()) + ".json")).lexically_normal();
                require(expected.insert(relative).second, "Manifest repeats a level file");
                result.push_back(relative);
            }
        }
        require(!result.empty(), "Manifest has no levels to qualify");
        std::set<std::filesystem::path> actual;
        for (const auto& directory : directories)
        {
            require(std::filesystem::is_directory(directory), "Manifest level directory is absent: " + directory.string());
            for (const auto& file : std::filesystem::directory_iterator(directory))
            {
                if (file.is_regular_file() && file.path().extension() == ".json")
                { actual.insert(file.path().lexically_relative(root).lexically_normal()); }
            }
        }
        require(actual == expected, "Manifest and packaged level-file inventories differ in " + content.string());
        return result;
    }

    void write_document(const std::filesystem::path& path, const rapidjson::Document& document)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(document.Accept(writer), "Cannot encode " + path.string());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
        output.close();
        require(output.good(), "Cannot write " + path.string());
    }

    void schema_is(const rapidjson::SchemaDocument& schema,
        const rapidjson::Document& document, bool expected, const std::string& name)
    {
        rapidjson::SchemaValidator validator(schema);
        const bool accepted = document.Accept(validator);
        if (accepted != expected)
        {
            rapidjson::StringBuffer pointer;
            validator.GetInvalidDocumentPointer().StringifyUriFragment(pointer);
            throw std::runtime_error(name + (expected ? " rejected by schema at " :
                " unexpectedly accepted by schema at ") + pointer.GetString());
        }
    }

    void compare_runtime(const LevelLoadedInfo& modern, const LevelLoadedInfo& legacy,
        const LevelObjectBuilder& builder)
    {
        require(modern.display_name() == legacy.display_name() &&
            modern.icon_frame() == legacy.icon_frame() &&
            modern.include_in_random() == legacy.include_in_random() &&
            modern.team_a_spawns() == legacy.team_a_spawns() &&
            modern.team_b_spawns() == legacy.team_b_spawns() &&
            modern.out_of_bounds_rectangle() == legacy.out_of_bounds_rectangle() &&
            modern.camera_bounds_rectangle() == legacy.camera_bounds_rectangle() &&
            modern.zoom_out_start_bounds_rectangle() == legacy.zoom_out_start_bounds_rectangle() &&
            modern.zoom_out_finish_bounds_rectangle() == legacy.zoom_out_finish_bounds_rectangle() &&
            modern.sound_bank_name() == legacy.sound_bank_name() &&
            modern.has_music() == legacy.has_music() && modern.music_name() == legacy.music_name() &&
            modern.music_volume() == legacy.music_volume() &&
            modern.has_end_sting() == legacy.has_end_sting() &&
            modern.end_sting_name() == legacy.end_sting_name() &&
            modern.end_sting_volume() == legacy.end_sting_volume(),
            "Format tag changed level metadata, spawns, bounds or sound bindings");
        const auto modern_stations = modern.stations();
        const auto legacy_stations = legacy.stations();
        require(modern_stations.size() == legacy_stations.size(), "Format tag changed station count");
        for (size_t index = 0; index < modern_stations.size(); ++index)
        {
            require(modern_stations[index].name == legacy_stations[index].name &&
                modern_stations[index].position == legacy_stations[index].position,
                "Format tag changed a named station or its order");
        }

        const TeamColour colours;
        const auto modern_collision = builder.build_collision_objects(modern.collision_objects_json(), colours);
        const auto legacy_collision = builder.build_collision_objects(legacy.collision_objects_json(), colours);
        require(modern_collision.size() == legacy_collision.size(), "Format tag changed collision object count");
        for (size_t index = 0; index < modern_collision.size(); ++index)
        {
            const CollisionObject& left = *modern_collision[index];
            const CollisionObject& right = *legacy_collision[index];
            require(typeid(left) == typeid(right) && left.bounds() == right.bounds() &&
                left.tag() == right.tag() && left.layer() == right.layer() && left.mask() == right.mask() &&
                left.shape()->shape_type() == right.shape()->shape_type() &&
                left.shape()->bounding_box() == right.shape()->bounding_box(),
                "Format tag changed a built object's type, bounds or collision classification");
            if (const auto* slab = dynamic_cast<const FixedSlabPaintable*>(&left))
            {
                const auto* peer = dynamic_cast<const FixedSlabPaintable*>(&right);
                require(peer != nullptr && slab->geometry().shape() == peer->geometry().shape() &&
                    slab->exposed_bottom() == peer->exposed_bottom(),
                    "Format tag changed the fixed slab collider or exposed shadow intervals");
            }
            const auto* left_paint = dynamic_cast<const PaintableObject*>(&left);
            const auto* right_paint = dynamic_cast<const PaintableObject*>(&right);
            require((left_paint == nullptr) == (right_paint == nullptr), "Format tag changed scoring participation");
            if (left_paint != nullptr)
            {
                const PaintTotal a = left_paint->paint_total();
                const PaintTotal b = right_paint->paint_total();
                require(a.total == b.total && a.team_a == b.team_a && a.team_b == b.team_b &&
                    a.player_areas == b.player_areas, "Format tag changed exposed paint coverage");
                std::vector<PaintSurfaceSample> a_samples, b_samples;
                left_paint->append_paint_surfaces(a_samples);
                right_paint->append_paint_surfaces(b_samples);
                require(a_samples.size() == b_samples.size(), "Format tag changed paint tile inventory");
                for (std::size_t tile = 0; tile < a_samples.size(); ++tile)
                {
                    const auto& a_tile = a_samples[tile]; const auto& b_tile = b_samples[tile];
                    require(a_tile.corners == b_tile.corners && a_tile.face == b_tile.face &&
                        a_tile.team == b_tile.team && a_tile.paint_owner == b_tile.paint_owner &&
                        a_tile.scored_area == b_tile.scored_area && a_tile.source_index == b_tile.source_index,
                        "Format tag changed an actual paint tile's pose, local face, credit or authoritative area");
                }
            }
        }
        const auto modern_visual = builder.build_non_collision_objects(modern.non_collision_objects_json());
        const auto legacy_visual = builder.build_non_collision_objects(legacy.non_collision_objects_json());
        require(modern_visual.size() == legacy_visual.size(), "Format tag changed decorative object count");
        for (size_t index = 0; index < modern_visual.size(); ++index)
        {
            require(typeid(*modern_visual[index]) == typeid(*legacy_visual[index]) &&
                modern_visual[index]->bounds() == legacy_visual[index]->bounds(),
                "Format tag changed a built decorative object's type or bounds");
        }
    }

    void runtime_rejects(const std::filesystem::path& path, const char* field,
        const LevelObjectBuilder& builder, const RenderResources& render, const AudioResources& audio)
    {
        bool named = false;
        try
        {
            const LevelLoadedInfo info(path.string().c_str());
            info.validate();
            info.validate_content(builder, render, audio);
        }
        catch (const std::runtime_error& error)
        {
            const std::string message = error.what();
            named = message.find(path.filename().string()) != std::string::npos &&
                message.find(field) != std::string::npos;
        }
        require(named, path.filename().string() + " must fail with its file and owning field named");
    }

    void check_versions(const std::filesystem::path& scratch)
    {
        require(level_format::current == 1, "Update compatibility acceptance when the format policy changes");
        size_t index = 0;
        for (const char* value : { "-1", "0", "2", "1.0", "1e0", "true", "null", "\"1\"",
            "[]", "{}", "2147483648", "18446744073709551616" })
        {
            const auto path = scratch / ("version-" + std::to_string(index++) + ".json");
            {
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                output << "{\"format\":" << value << '}';
                output.close();
                require(output.good(), "Cannot write version rejection fixture");
            }
            bool named = false;
            try { const LevelLoadedInfo invalid(path.string().c_str()); }
            catch (const std::runtime_error& error)
            {
                const std::string message = error.what();
                named = message.find(path.filename().string()) != std::string::npos &&
                    message.find("format") != std::string::npos &&
                    message.find("display_name") == std::string::npos;
            }
            require(named, std::string("Malformed/future format ") + value +
                " must fail before any content field is interpreted");
        }
        // A valid or absent tag passes construction even if later validation
        // will reject the incomplete content. The tag is the first boundary.
        for (const char* source : { "{}", "{\"format\":1}" })
        {
            rapidjson::Document document;
            document.Parse(source);
            const auto path = scratch / "supported-version-only.json";
            write_document(path, document);
            const LevelLoadedInfo supported(path.string().c_str());
        }
    }

    void check_schema_fixtures(const rapidjson::SchemaDocument& schema,
        const rapidjson::Document& source, const std::filesystem::path& scratch,
        const LevelObjectBuilder& builder, const RenderResources& render, const AudioResources& audio)
    {
        const auto variant = [&](const char* name, bool expected, auto edit)
        {
            auto document = copy_document(source);
            edit(document);
            schema_is(schema, document, expected, name);
            const auto path = scratch / (std::string(name) + ".json");
            write_document(path, document);
            return path;
        };
        const auto unknown = variant("unknown-metadata", true, [](rapidjson::Document& d)
        {
            d.AddMember("future_author_note", 42, d.GetAllocator());
            d["collision_objects"][0].AddMember("editor_selection", true, d.GetAllocator());
            d["stations"][0].AddMember("route_note", "retained metadata", d.GetAllocator());
        });
        const LevelLoadedInfo baseline((scratch / "baseline.json").string().c_str());
        const LevelLoadedInfo extras(unknown.string().c_str());
        extras.validate();
        extras.validate_content(builder, render, audio);
        compare_runtime(baseline, extras, builder);

        const auto alternate = variant("rectangle-tlbr", true, [](rapidjson::Document& d)
        {
            auto& object = d["collision_objects"][0];
            const auto& rectangle = object["rectangle"];
            rapidjson::Value tlbr(rapidjson::kObjectType);
            tlbr.AddMember("left", rectangle["x"].GetDouble(), d.GetAllocator());
            tlbr.AddMember("top", rectangle["y"].GetDouble(), d.GetAllocator());
            tlbr.AddMember("right", rectangle["x"].GetDouble() + rectangle["width"].GetDouble(), d.GetAllocator());
            tlbr.AddMember("bottom", rectangle["y"].GetDouble() + rectangle["height"].GetDouble(), d.GetAllocator());
            object.AddMember("rectangle_tlbr", tlbr, d.GetAllocator());
            object.RemoveMember("rectangle");
        });
        const LevelLoadedInfo alternate_info(alternate.string().c_str());
        alternate_info.validate();
        alternate_info.validate_content(builder, render, audio);
        compare_runtime(baseline, alternate_info, builder);

        const auto both = variant("rectangle-precedence", true, [](rapidjson::Document& d)
        {
            // The runtime deliberately ignores the alternative when rectangle
            // exists; the schema must preserve that compatibility, too.
            d["collision_objects"][0].AddMember("rectangle_tlbr", "ignored", d.GetAllocator());
        });
        const LevelLoadedInfo both_info(both.string().c_str());
        both_info.validate();
        both_info.validate_content(builder, render, audio);
        compare_runtime(baseline, both_info, builder);

        variant("unsupported-format", false, [](rapidjson::Document& d) { d["format"].SetInt(2); });
        variant("string-format", false, [](rapidjson::Document& d) { d["format"].SetString("1"); });
        variant("floating-format", false, [](rapidjson::Document& d) { d["format"].SetDouble(1.0); });
        variant("missing-display-name", false, [](rapidjson::Document& d) { d.RemoveMember("display_name"); });
        variant("short-team", false, [](rapidjson::Document& d) { d["spawn_locations"]["team_a"].PopBack(); });
        variant("bad-bounds", false, [](rapidjson::Document& d) { d["level_bounds"]["camera_bounds"]["width"].SetInt(0); });
        variant("bad-random", false, [](rapidjson::Document& d) { d["include_in_random"].SetString("false"); });
        variant("bad-object-type", false, [](rapidjson::Document& d) { d["collision_objects"][0]["type"].SetString("Visual"); });
        variant("bad-collision-tag", false, [](rapidjson::Document& d)
            { d["collision_objects"][0]["collision_type"].SetString("STRUCTURE_PAINTABLE"); });
        variant("missing-geometry", false, [](rapidjson::Document& d) { d["collision_objects"][0].RemoveMember("rectangle"); });
        variant("bad-visual-type", false, [](rapidjson::Document& d) { d["non_collision_objects"][0]["type"].SetString("Structure"); });
        variant("bad-paint-face", false, [](rapidjson::Document& d)
            { d["collision_objects"][3]["paintable_faces"]["top"].SetInt(1); });
        variant("missing-paint-face", false, [](rapidjson::Document& d)
            { d["collision_objects"][3]["paintable_faces"].RemoveMember("bottom"); });
        for (const char* field : { "music_name", "music_volume", "end_sting_name", "end_sting_volume" })
        {
            variant((std::string("unpaired-") + field).c_str(), false,
                [field](rapidjson::Document& d) { d["sound"].RemoveMember(field); });
        }
        variant("bad-sting-volume", false, [](rapidjson::Document& d) { d["sound"]["end_sting_volume"].SetDouble(1.1); });
        variant("bad-music-volume", false, [](rapidjson::Document& d) { d["sound"]["music_volume"].SetDouble(1.5); });
        variant("reserved-station", false, [](rapidjson::Document& d) { d["stations"][0]["name"].SetString("spawn_a"); });
        // The template's left wall is a ground STRUCTURE and its floor a ground STRUCTURE_PAINTABLE.
        variant("ruler-jump-through", true, [](rapidjson::Document& d)
        {
            d["collision_objects"][0]["collision_type"].SetString("STRUCTURE_JUMP_THROUGH");
            d["collision_objects"][0]["material"].SetString("ruler");
        });
        const auto scored_ruler = variant("paintable-ruler-jump-through", true, [](rapidjson::Document& d)
        {
            rapidjson::Value& object = d["collision_objects"][3];
            object["collision_type"].SetString("STRUCTURE_JUMP_THROUGH");
            object["material"].SetString("ruler");
            for (const char* face : { "left", "bottom", "right" })
            { object["paintable_faces"][face].SetBool(false); }
        });
        const LevelLoadedInfo scored_ruler_info(scored_ruler.string().c_str());
        scored_ruler_info.validate();
        scored_ruler_info.validate_content(builder, render, audio);
        const auto scored_ruler_objects = builder.build_collision_objects(
            scored_ruler_info.collision_objects_json(), TeamColour{});
        require(to_collision_type(scored_ruler_objects[3]->tag()) == CollisionObjectType::structure_jump_through &&
            dynamic_cast<const PaintableObject*>(scored_ruler_objects[3].get()) != nullptr,
            "Paintable ruler lost its independent scoring or jump-through collision role");
        variant("paintable-jump-material-mismatch", false, [](rapidjson::Document& d)
        {
            d["collision_objects"][3]["collision_type"].SetString("STRUCTURE_JUMP_THROUGH");
            d["collision_objects"][3]["material"].SetString("ground");
        });
        variant("no-material", true, [](rapidjson::Document& d) { d["collision_objects"][0].RemoveMember("material"); });
        variant("bad-material", false, [](rapidjson::Document& d) { d["collision_objects"][0]["material"].SetString("chalk"); });
        variant("non-string-material", false, [](rapidjson::Document& d) { d["collision_objects"][0]["material"].SetInt(1); });
        variant("material-type-mismatch", false, [](rapidjson::Document& d)
            { d["collision_objects"][0]["material"].SetString("ruler"); });
        variant("paintable-material-mismatch", false, [](rapidjson::Document& d)
            { d["collision_objects"][3]["material"].SetString("set_square"); });

        const auto outside = variant("outside-spawn", true, [](rapidjson::Document& d)
            { d["spawn_locations"]["team_a"][0]["x"].SetInt(-100000); });
        runtime_rejects(outside, "out_of_bounds", builder, render, audio);
        const auto unknown_sheet = variant("unknown-sheet", true, [](rapidjson::Document& d)
            { d["collision_objects"][0]["sheet_name"].SetString("not_a_loaded_sheet"); });
        runtime_rejects(unknown_sheet, "collision_objects", builder, render, audio);
        const auto chromatic = variant("chromatic-hex", true, [](rapidjson::Document& d)
            { d["collision_objects"][0]["colour"].SetString("FF0000"); });
        runtime_rejects(chromatic, "not a grey", builder, render, audio);
    }

    // A team gate (ROAD#15): "gate": "paint" on a solid static paintable.
    void check_gate_fixtures(const rapidjson::SchemaDocument& schema,
        const rapidjson::Document& source, const std::filesystem::path& scratch,
        const LevelObjectBuilder& builder, RenderResources& render, const AudioResources& audio,
        const GameTuning& tuning)
    {
        auto baseline = copy_document(source);
        auto& allocator = baseline.GetAllocator();
        rapidjson::Value gate;
        gate.CopyFrom(baseline["collision_objects"][3], allocator);
        gate["rectangle"]["x"].SetInt(1460);
        gate["rectangle"]["y"].SetInt(960);
        gate["rectangle"]["width"].SetInt(80);
        gate["rectangle"]["height"].SetInt(240);
        gate["material"].SetString("eraser");
        gate["paintable_faces"]["left"].SetBool(true);
        gate["paintable_faces"]["right"].SetBool(true);
        gate.AddMember("gate", "paint", allocator);
        baseline["collision_objects"].PushBack(gate, allocator);
        const rapidjson::SizeType at = baseline["collision_objects"].Size() - 1;

        const auto check = [&](const char* name, bool schema_accepts, bool runtime_accepts, auto edit)
        {
            auto document = copy_document(baseline);
            edit(document);
            schema_is(schema, document, schema_accepts, name);
            const auto path = scratch / (std::string(name) + ".json");
            write_document(path, document);
            if (!runtime_accepts)
            {
                runtime_rejects(path, "gate", builder, render, audio);
                return;
            }
            const LevelLoadedInfo info(path.string().c_str());
            info.validate();
            validate_stage_objects(tuning, builder, render, audio, info);
            const auto objects = builder.build_collision_objects(info.collision_objects_json(), TeamColour{});
            const auto* built = dynamic_cast<const TeamGateStructurePaintable*>(objects[at].get());
            require(built != nullptr, "An authored gate was silently built as a plain paintable");
            require(built->bounds() == RectangleF(1460.0f, 960.0f, 80.0f, 240.0f) &&
                built->open_for() == PlayerTeam::none && built->paint_leader() == PlayerTeam::none &&
                built->paint_total().total > 0.0f, "A gate's rectangle, closed start or paintable area changed");
        };
        check("gate-solid", true, true, [](rapidjson::Document&) {});
        check("gate-metadata", true, true, [&](rapidjson::Document& d)
        { d["collision_objects"][at].AddMember("_comment", "retained metadata", d.GetAllocator()); });
        check("gate-one-face", true, true, [&](rapidjson::Document& d)
        {
            auto& faces = d["collision_objects"][at]["paintable_faces"];
            faces["left"].SetBool(false); faces["right"].SetBool(false);
        });
        check("gate-unknown", false, false, [&](rapidjson::Document& d)
        { d["collision_objects"][at]["gate"].SetString("wind"); });
        check("gate-number", false, false, [&](rapidjson::Document& d)
        { d["collision_objects"][at]["gate"].SetInt(1); });
        check("gate-one-way", false, false, [&](rapidjson::Document& d)
        {
            d["collision_objects"][at]["collision_type"].SetString("STRUCTURE_JUMP_THROUGH");
            d["collision_objects"][at]["material"].SetString("ruler");
        });
        check("gate-plain-structure", false, false, [&](rapidjson::Document& d)
        {
            auto& object = d["collision_objects"][at];
            object["type"].SetString("Structure");
            object["collision_type"].SetString("STRUCTURE");
            object["material"].SetString("ground");
            object.RemoveMember("paintable_faces");
        });
        check("gate-no-face", true, false, [&](rapidjson::Document& d)
        {
            for (const char* face : { "left", "top", "right", "bottom" })
            { d["collision_objects"][at]["paintable_faces"][face].SetBool(false); }
        });
        check("gate-moving", false, false, [&](rapidjson::Document& d)
        {
            rapidjson::Value motion(rapidjson::kObjectType);
            motion.AddMember("type", "horizontal", d.GetAllocator());
            motion.AddMember("travel_x", 40, d.GetAllocator());
            motion.AddMember("period", 4, d.GetAllocator());
            motion.AddMember("phase", 0, d.GetAllocator());
            d["collision_objects"][at].AddMember("motion", motion, d.GetAllocator());
        });
        check("gate-on-visual", false, false, [&](rapidjson::Document& d)
        {
            rapidjson::Value visual(rapidjson::kObjectType);
            visual.CopyFrom(d["non_collision_objects"][0], d.GetAllocator());
            visual.AddMember("gate", "paint", d.GetAllocator());
            d["non_collision_objects"].PushBack(visual, d.GetAllocator());
        });

        // The startup check: no spawn body may overlap a gate.
        auto spawned = copy_document(baseline);
        spawned["collision_objects"][at]["rectangle"]["x"].SetInt(120);
        spawned["collision_objects"][at]["rectangle"]["y"].SetInt(1000);
        spawned["collision_objects"][at]["rectangle"]["height"].SetInt(200);
        schema_is(schema, spawned, true, "gate-over-spawn");
        const auto spawn_path = scratch / "gate-over-spawn.json";
        write_document(spawn_path, spawned);
        bool refused = false;
        try
        {
            const LevelLoadedInfo info(spawn_path.string().c_str());
            info.validate();
            info.validate_content(builder, render, audio);
            validate_stage_objects(tuning, builder, render, audio, info);
        }
        catch (const std::runtime_error& error)
        {
            const std::string message = error.what();
            refused = message.find("gate-over-spawn.json") != std::string::npos &&
                message.find("a gate must not overlap a spawn body") != std::string::npos;
        }
        require(refused, "A gate over a spawn body passed startup validation");
        std::cout << "PASS team gate format: solid static paintable gates load closed; malformed, one-way, plain, "
            "unpainted, moving, decorative and spawn-covering gates are refused with their file named\n";
    }

    void check_motion_fixtures(const rapidjson::SchemaDocument& schema,
        const rapidjson::Document& source, const std::filesystem::path& scratch,
        const LevelObjectBuilder& builder, const RenderResources& render, const AudioResources& audio)
    {
        auto baseline = copy_document(source);
        auto& allocator = baseline.GetAllocator();
        rapidjson::Value mover;
        mover.CopyFrom(baseline["collision_objects"][3], allocator);
        mover["rectangle"]["x"].SetInt(600);
        mover["rectangle"]["y"].SetInt(700);
        mover["rectangle"]["width"].SetInt(320);
        mover["rectangle"]["height"].SetInt(20);
        rapidjson::Value motion(rapidjson::kObjectType);
        motion.AddMember("type", "horizontal", allocator);
        motion.AddMember("travel_x", 120, allocator);
        motion.AddMember("period", 6.4, allocator);
        motion.AddMember("phase", 0.25, allocator);
        mover.AddMember("motion", motion, allocator);
        baseline["collision_objects"].Clear();
        baseline["collision_objects"].PushBack(mover, allocator);

        const auto check = [&](const char* name, bool schema_accepts, bool runtime_accepts, auto edit)
        {
            auto document = copy_document(baseline);
            edit(document);
            schema_is(schema, document, schema_accepts, name);
            const auto path = scratch / (std::string(name) + ".json");
            write_document(path, document);
            if (!runtime_accepts)
            {
                runtime_rejects(path, "motion", builder, render, audio);
                return;
            }
            const LevelLoadedInfo info(path.string().c_str());
            info.validate(); info.validate_content(builder, render, audio);
            const auto objects = builder.build_collision_objects(info.collision_objects_json(), TeamColour{});
            const auto* moving = dynamic_cast<const TranslatingStructurePaintable*>(objects[0].get());
            require(moving != nullptr, "Authored motion was silently built as static terrain");
            const auto state = moving->motion_state();
            require(state.spec.anchor == RectangleF(600.0f, 700.0f, 320.0f, 20.0f) &&
                state.spec.travel_x == 120.0f && state.spec.period_seconds == 6.4f &&
                state.spec.initial_phase == 0.25f && moving->bounds().x == 660.0f &&
                moving->motion_id() == 0, "Motion fields, initial phase or admission boundary changed");
            const auto& fields = document["collision_objects"][0]["motion"];
            const bool paint = fields.HasMember("drive") && std::string(fields["drive"].GetString()) == "paint";
            require(state.spec.drive == (paint ? HorizontalDrive::paint : HorizontalDrive::cycle) &&
                (!paint || state.spec.team_a_heading ==
                    (std::string(fields["team_a_heads"].GetString()) == "right" ? 1 : -1)),
                "Motion drive or paint heading changed");
        };
        check("motion-solid", true, true, [](rapidjson::Document&) {});
        check("motion-jump-through", true, true, [](rapidjson::Document& d)
        {
            d["collision_objects"][0]["collision_type"].SetString("STRUCTURE_JUMP_THROUGH");
            d["collision_objects"][0]["material"].SetString("ruler");
        });
        check("motion-metadata", true, true, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"].AddMember("_comment", "retained metadata", d.GetAllocator()); });
        check("motion-tlbr", true, true, [](rapidjson::Document& d)
        {
            auto& object = d["collision_objects"][0];
            rapidjson::Value tlbr(rapidjson::kObjectType);
            tlbr.AddMember("left", 600, d.GetAllocator()); tlbr.AddMember("top", 700, d.GetAllocator());
            tlbr.AddMember("right", 920, d.GetAllocator()); tlbr.AddMember("bottom", 720, d.GetAllocator());
            object.RemoveMember("rectangle"); object.AddMember("rectangle_tlbr", tlbr, d.GetAllocator());
        });
        check("motion-rectangle-precedence", true, true, [](rapidjson::Document& d)
        { d["collision_objects"][0].AddMember("rectangle_tlbr", "ignored", d.GetAllocator()); });
        const auto drive = [](rapidjson::Document& d, const char* value, const char* heads)
        {
            auto& fields = d["collision_objects"][0]["motion"];
            fields.AddMember("drive", rapidjson::StringRef(value), d.GetAllocator());
            if (heads != nullptr) { fields.AddMember("team_a_heads", rapidjson::StringRef(heads), d.GetAllocator()); }
        };
        check("motion-cycle-drive", true, true, [&](rapidjson::Document& d) { drive(d, "cycle", nullptr); });
        check("motion-paint-drive-left", true, true, [&](rapidjson::Document& d) { drive(d, "paint", "left"); });
        check("motion-paint-drive-right", true, true, [&](rapidjson::Document& d) { drive(d, "paint", "right"); });
        check("motion-paint-no-heading", true, false, [&](rapidjson::Document& d) { drive(d, "paint", nullptr); });
        check("motion-paint-bad-heading", false, false, [&](rapidjson::Document& d) { drive(d, "paint", "up"); });
        check("motion-unknown-drive", false, false, [&](rapidjson::Document& d) { drive(d, "wind", nullptr); });
        check("motion-cycle-heading", true, false, [&](rapidjson::Document& d) { drive(d, "cycle", "right"); });
        check("motion-paint-return-leg", true, false, [&](rapidjson::Document& d)
        {
            drive(d, "paint", "right");
            d["collision_objects"][0]["motion"]["phase"].SetDouble(0.75);
        });
        for (const char* field : { "type", "travel_x", "period", "phase" })
        {
            check((std::string("motion-missing-") + field).c_str(), false, false,
                [field](rapidjson::Document& d) { d["collision_objects"][0]["motion"].RemoveMember(field); });
        }
        check("motion-wrong-type", false, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"].SetBool(false); });
        check("motion-unsupported-kind", false, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"]["type"].SetString("vertical"); });
        check("motion-string-travel", false, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"]["travel_x"].SetString("120"); });
        size_t travel_case = 0;
        for (const double travel : { 0.0, -120.0, 1.0e-40, 1.0e40 })
        {
            const bool structural = travel > 0.0;
            check((std::string("motion-travel-") + std::to_string(travel_case++)).c_str(), structural, false,
                [travel](rapidjson::Document& d) { d["collision_objects"][0]["motion"]["travel_x"].SetDouble(travel); });
        }
        check("motion-short-period", false, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"]["period"].SetDouble(SECONDS_PER_STEP); });
        check("motion-overflow-period", true, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"]["period"].SetDouble(1.0e40); });
        check("motion-negative-phase", false, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"]["phase"].SetDouble(-0.1); });
        check("motion-phase-one", false, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"]["phase"].SetDouble(1.0); });
        check("motion-phase-rounding", true, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["motion"]["phase"].SetDouble(0.999999999999); });
        check("motion-indistinct-edges", true, false, [](rapidjson::Document& d)
        { d["collision_objects"][0]["rectangle"]["x"].SetDouble(1.0e20); });
        for (const char* field : { "axis", "travel_y", "rotation" })
        {
            check((std::string("motion-unsupported-") + field).c_str(), false, false,
                [field](rapidjson::Document& d)
                { d["collision_objects"][0]["motion"].AddMember(rapidjson::StringRef(field), 0, d.GetAllocator()); });
        }
        check("motion-on-plain-structure", false, false, [](rapidjson::Document& d)
        {
            d["collision_objects"][0]["type"].SetString("Structure");
            d["collision_objects"][0]["collision_type"].SetString("STRUCTURE");
        });
        check("motion-on-ramp", false, false, [](rapidjson::Document& d)
        {
            d["collision_objects"][0]["type"].SetString("StructureRamp");
            d["collision_objects"][0]["collision_type"].SetString("STRUCTURE_RAMP_RIGHT");
        });
        check("motion-on-visual", false, false, [](rapidjson::Document& d)
        {
            auto& object = d["non_collision_objects"][0];
            rapidjson::Value motion; motion.CopyFrom(d["collision_objects"][0]["motion"], d.GetAllocator());
            object.AddMember("motion", motion, d.GetAllocator());
            d["collision_objects"][0].RemoveMember("motion");
        });
        std::cout << "PASS horizontal motion schema/runtime fields, diagnostics and initial construction\n";
    }

    // The editor's warning-only resource check reads the sheet and bank formats
    // itself. Hold its catalog to the engine's loaders on the staged content.
    void check_editor_catalog(const std::filesystem::path& content)
    {
        const colourwars_editor::ContentCatalog catalog = colourwars_editor::load_content_catalog(content);
        require(catalog.manifest_read && catalog.problems.empty() && !catalog.sprite_frames.empty() &&
            !catalog.sound_effects.empty(), "The editor's content catalog did not read the staged manifest");
        for (const auto& [sheet, frames] : catalog.sprite_frames)
        {
            require(frames.has_value(), "The editor could not read sprite sheet " + sheet);
            const std::filesystem::path path = content / "textures" / (sheet + ".json");
            const auto loaded = read_sprite_sheet(path.string().c_str(), TextureHandle{});
            for (const std::string& frame : *frames)
            {
                try { loaded->resolve_sprite_frame(frame); }
                catch (const std::exception&)
                {
                    throw std::runtime_error("The editor accepts frame " + frame + " that the engine's " +
                        sheet + " does not resolve");
                }
            }
            // A strip is not a frame to the engine, so the editor must not take one.
            const rapidjson::Document json = read_document(path);
            for (const auto& strip : json["animation_strips"].GetArray())
            {
                const std::string name = strip["name"].GetString();
                if (frames->contains(name)) { continue; }
                bool refused = false;
                try { loaded->resolve_sprite_frame(name); }
                catch (const std::exception&) { refused = true; }
                require(refused, "The engine resolves strip " + name + " as a frame the editor would refuse");
            }
        }
        for (const auto& [bank, effects] : catalog.sound_effects)
        {
            require(effects.has_value(), "The editor could not read sound bank " + bank);
            const SoundBankDefinition definition = read_sound_bank_definition(
                (content / "sounds" / (bank + ".json")).string().c_str());
            std::set<std::string> engine;
            for (const SoundBankDefinition::Effect& effect : definition.effects) { engine.insert(effect.name); }
            require(engine == *effects, "The editor's effect names for " + bank +
                " differ from the engine's sound-bank definition");
        }
        std::cout << "PASS editor content catalog matches the engine's sprite-sheet frames and sound-bank effects\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("level format", [&]
    {
        require(argc == 3, "Usage: level_format.exe <staged content directory> <repository directory>");
        const std::filesystem::path content(argv[1]);
        const std::filesystem::path repo(argv[2]);
        const auto scratch = std::filesystem::absolute(argv[0]).parent_path() / "level-format-fixtures";
        std::filesystem::create_directories(scratch);
        const auto schema_json = read_document(repo / "docs/level.schema.json");
        const rapidjson::SchemaDocument schema(schema_json);
        RenderResources render;
        render.add_sprite_sheet("sprite_sheet_1", read_sprite_sheet(
            (content / "textures/sprite_sheet_1.json").string().c_str(), TextureHandle{}));
        render.add_sprite_sheet("paint_fx", read_sprite_sheet(
            (content / "textures/paint_fx.json").string().c_str(), TextureHandle{}));
        render.add_sprite_sheet("terrain", read_sprite_sheet(
            (content / "textures/terrain.json").string().c_str(), TextureHandle{}));
        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const PlaySettings preferences;
        const LevelObjectBuilder builder(&render, &audio, &preferences, &tuning.paint());

        size_t checked = 0;
        const auto check_compatible = [&](const std::filesystem::path& path)
        {
            auto document = read_document(path);
            require(document.HasMember("format") && document["format"].IsInt() &&
                document["format"].GetInt() == level_format::current,
                path.string() + " needs an explicit current format tag");
            schema_is(schema, document, true, path.string());
            const LevelLoadedInfo modern(path.string().c_str());
            modern.validate();
            modern.validate_content(builder, render, audio);
            document.RemoveMember("format");
            schema_is(schema, document, true, path.string() + " untagged legacy clone");
            const auto legacy_path = scratch / ("legacy-" + std::to_string(checked) + ".json");
            write_document(legacy_path, document);
            const LevelLoadedInfo legacy(legacy_path.string().c_str());
            legacy.validate();
            legacy.validate_content(builder, render, audio);
            compare_runtime(modern, legacy, builder);
            ++checked;
        };
        const auto staged_levels = manifest_levels(content);
        const auto source_levels = manifest_levels(repo / "game/content");
        require(staged_levels == source_levels, "Source and staged manifests differ in level inventory or order");
        require(staged_levels.size() == 11, "Current format acceptance needs all eleven registered stages");
        for (const auto& file : staged_levels)
        {
            check_compatible(content / file);
            check_compatible(repo / "game/content" / file);
            const LevelLoadedInfo staged((content / file).string().c_str());
            const LevelLoadedInfo source((repo / "game/content" / file).string().c_str());
            compare_runtime(staged, source, builder);
        }
        const auto template_path = repo / "editor/template.json";
        check_compatible(template_path);
        const auto source = read_document(template_path);
        write_document(scratch / "baseline.json", source);
        check_versions(scratch);
        check_schema_fixtures(schema, source, scratch, builder, render, audio);
        check_motion_fixtures(schema, source, scratch, builder, render, audio);
        check_gate_fixtures(schema, source, scratch, builder, render, audio, tuning);
        check_editor_catalog(content);
        std::cout << "PASS level format 1: " << checked << " staged/source/template files, legacy object/paint parity, "
            "early version rejection, schema fixtures and runtime resource/geometry authority.\n";
        return 0;
    });
}
