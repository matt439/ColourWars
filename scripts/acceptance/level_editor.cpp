#include "editor/level_document.h"
#include "editor/blockout_aids.h"
#include "game/paint_face_geometry.h"
#include "game/level_format.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <Windows.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace
{
    using namespace colourwars_editor;
    namespace fs = std::filesystem;

    void require(bool condition, const std::string& message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    std::string contents(const fs::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Could not read acceptance input");
        return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
    }

    void write(const fs::path& path, const std::string& text)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream << text;
        stream.close();
        require(stream.good(), "Could not write acceptance fixture");
    }

    rapidjson::Document parse(const std::string& text)
    {
        rapidjson::Document result;
        result.Parse(text.data(), text.size());
        require(!result.HasParseError() && result.IsObject(), "Output is not a JSON object");
        return result;
    }

    std::string encode(const rapidjson::Document& document)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(document.Accept(writer), "Could not encode format-version fixture");
        return std::string(buffer.GetString(), buffer.GetSize());
    }

    bool equal(const Rect& actual, const Rect& expected)
    {
        constexpr double tolerance = 0.000001;
        return std::abs(actual.x - expected.x) < tolerance &&
            std::abs(actual.y - expected.y) < tolerance &&
            std::abs(actual.width - expected.width) < tolerance &&
            std::abs(actual.height - expected.height) < tolerance;
    }

    Entry entry(const LevelDocument& document, EntryKind kind, std::size_t index = 0)
    {
        for (const Entry& candidate : document.entries())
            if (candidate.kind == kind && candidate.index == index) return candidate;
        throw std::runtime_error("Expected entry is absent");
    }

    template<typename Action>
    void reject_without_change(LevelDocument& document, Action action, const char* message)
    {
        const std::string before = document.serialize();
        const fs::path path = document.path();
        const bool dirty = document.dirty();
        const bool undo = document.can_undo();
        const bool redo = document.can_redo();
        bool rejected = false;
        try { action(); }
        catch (const std::exception& error)
        {
            rejected = true;
            require(std::string(error.what()).size() > 0, "Failure needs a diagnostic");
        }
        require(rejected, message);
        require(document.serialize() == before && document.path() == path &&
            document.dirty() == dirty && document.can_undo() == undo &&
            document.can_redo() == redo, "Failed operation changed document or history");
    }

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

    void check_shipped_levels(const fs::path& repo, const fs::path& scratch)
    {
        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters",
            "movement_course" })
        {
            const fs::path source = repo / "game/content/levels" / (std::string(name) + ".json");
            const std::string original = contents(source);
            LevelDocument document;
            document.load(source);
            require(!document.dirty() && !document.can_undo() && !document.can_redo(),
                "Loading must start a clean document history");
            require(document.validate().empty(), "Shipped level did not validate");
            const std::map<std::string, double> runtime_areas = {
                {"king_of_the_hill", 205920.0}, {"turbulence", 249000.0},
                {"close_quarters", 91200.0}, {"movement_course", 194880.0}};
            const BlockoutTuning tuning = load_blockout_tuning(repo / "game/content/tuning.json",
                repo / "editor/movement-baseline.json");
            require(std::abs(scoring_area(scoring_faces(document, tuning)) - runtime_areas.at(name)) < 0.1,
                "Editor exposed scoring area differs from actual runtime stage measurements");
            require(parse(original) == parse(document.serialize()),
                "Loading changed source level data");
            const std::string before_no_op = document.serialize();
            const Entry platform = entry(document, EntryKind::collision);
            document.set_rectangle(platform, document.rectangle(platform));
            require(document.serialize() == before_no_op && !document.dirty() && !document.can_undo(),
                "Setting an unchanged rectangle created an edit or changed numeric spelling");

            const fs::path copy = scratch / (std::string(name) + ".json");
            document.save(copy);
            LevelDocument reopened;
            reopened.load(copy);
            require(parse(original) == parse(reopened.serialize()),
                "Saving and reopening changed source level data");
            require(contents(source) == original, "Round trip changed the shipped source");
            require(reopened.entries().size() == document.entries().size(),
                "Round trip changed selectable entries");
            for (const Entry& item : reopened.entries())
            {
                const Rect rectangle = reopened.rectangle(item);
                require(std::isfinite(rectangle.x) && std::isfinite(rectangle.y) &&
                    rectangle.width > 0 && rectangle.height > 0,
                    "A selectable entry has unusable canvas geometry");
                require(!reopened.properties(item).empty(), "A selectable entry has no properties");
            }
        }
        const fs::path template_path = repo / "editor/template.json";
        const std::string template_bytes = contents(template_path);
        const rapidjson::Document template_json = parse(template_bytes);
        require(template_json.HasMember("format") && template_json["format"].IsInt() &&
            template_json["format"].GetInt() == level_format::current,
            "Canonical editor template does not declare the shared current format");
        LevelDocument example;
        example.load(template_path);
        require(example.validate().empty() && !example.dirty(), "Canonical template did not load cleanly in editor");
        example.save(scratch / "template-roundtrip.json");
        LevelDocument template_reopened;
        template_reopened.load(example.path());
        require(parse(template_reopened.serialize()) == template_json && !template_reopened.dirty() &&
            contents(template_path) == template_bytes,
            "Canonical template editor round trip changed level fields or the source");
        std::cout << "PASS shipped levels and canonical editor template round trip and source preservation\n";
    }

    void check_format_versions(const fs::path& scratch)
    {
        LevelDocument document;
        const rapidjson::Document created = parse(document.serialize());
        require(created.HasMember("format") && created["format"].IsInt() &&
            created["format"].GetInt() == level_format::current,
            "New document did not declare the shared current level format");
        std::string extended = document.serialize();
        extended.insert(extended.find('{') + 1,
            "\n\"author_extension\": {\"format\":\"keep me\",\"flags\":[1,true,null],\"nested\":{\"x\":2}},");
        rapidjson::Document legacy = parse(extended);
        legacy.RemoveMember("format");
        const fs::path legacy_path = scratch / "legacy-format.json";
        const std::string legacy_bytes = encode(legacy);
        write(legacy_path, legacy_bytes);
        document.load(legacy_path);
        require(parse(document.serialize()) == legacy && !document.dirty() &&
            !document.can_undo() && !document.can_redo(),
            "Loading a legacy level changed its source or saved/history state");

        // Reach the filesystem failure after preparing the upgrade candidate.
        // Neither an omitted format nor unknown data may leak into live state.
        const fs::path blocked = scratch / "format-read-only.json";
        write(blocked, "retain existing destination");
        const DWORD attributes = GetFileAttributesW(blocked.c_str());
        require(attributes != INVALID_FILE_ATTRIBUTES &&
            SetFileAttributesW(blocked.c_str(), attributes | FILE_ATTRIBUTE_READONLY) != 0,
            "Could not create read-only format save fixture");
        try
        {
            reject_without_change(document, [&] { document.save(blocked); },
                "Legacy upgrade saved over a read-only destination");
            require(contents(blocked) == "retain existing destination" && contents(legacy_path) == legacy_bytes,
                "Failed legacy upgrade changed existing filesystem data");
        }
        catch (...)
        {
            SetFileAttributesW(blocked.c_str(), attributes);
            throw;
        }
        require(SetFileAttributesW(blocked.c_str(), attributes) != 0,
            "Could not restore format fixture attributes");
        const fs::path upgraded_path = scratch / "upgraded-format.json";
        document.save(upgraded_path);
        legacy.AddMember("format", level_format::current, legacy.GetAllocator());
        require(parse(contents(upgraded_path)) == legacy && parse(document.serialize()) == legacy &&
            !document.dirty() && !document.can_undo() && !document.can_redo(),
            "Successful legacy save did not upgrade just format and retain a clean history");
        require(contents(legacy_path) == legacy_bytes, "Save As upgrade changed the legacy source");
        LevelDocument reopened;
        reopened.load(upgraded_path);
        reopened.save(scratch / "current-format-roundtrip.json");
        require(parse(reopened.serialize()) == legacy && parse(contents(reopened.path())) == legacy &&
            !reopened.dirty(), "Explicit current format lost unknown fields on round trip");

        // Retain both undo and redo branches while failed loads try malformed
        // versions. Other required content is deliberately absent: the version
        // error must win, and every failure must identify the file and format.
        const Entry level = entry(reopened, EntryKind::level);
        reopened.set_property(level, "/display_name", "Format history first");
        const std::string first_edit = reopened.serialize();
        reopened.set_property(level, "/display_name", "Format history second");
        const std::string second_edit = reopened.serialize();
        require(reopened.undo() && reopened.can_undo() && reopened.can_redo(),
            "Version failure fixture needs both undo and redo history");
        int index = 0;
        for (const char* version : { "0", "2", "-1", "2147483647", "2147483648",
            "18446744073709551615", "18446744073709551616", "1e100", "1.5", "1.0", "1e0",
            "true", "false", "\"1\"", "null", "[]", "{}" })
        {
            const fs::path invalid = scratch / ("invalid-format-" + std::to_string(index++) + ".json");
            write(invalid, std::string("{\"format\":") + version + ",\"display_name\":null}");
            std::string diagnostic;
            reject_without_change(reopened, [&]
            {
                try { reopened.load(invalid); }
                catch (const std::exception& error) { diagnostic = error.what(); throw; }
            }, "Editor accepted an unsupported or malformed level format");
            require(diagnostic.find(invalid.filename().string()) != std::string::npos &&
                diagnostic.find("format") != std::string::npos &&
                diagnostic.find("display_name") == std::string::npos,
                "Version rejection did not report its file and format before decoding other fields");
        }
        require(reopened.serialize() == first_edit && reopened.redo() && reopened.serialize() == second_edit &&
            reopened.undo() && reopened.serialize() == first_edit,
            "Rejected versions altered undo/redo snapshots");
        for (const char* version : { "0", "2", "-1", "1.5", "1.0", "1e0", "2147483648" })
        {
            reject_without_change(reopened, [&] { reopened.set_property(level, "/format", version); },
                "Inspector accepted a format that the loader rejects");
        }

        document.load(legacy_path);
        const Entry legacy_level = entry(document, EntryKind::level);
        document.set_property(legacy_level, "/display_name", "Legacy first");
        const std::string legacy_first = document.serialize();
        document.set_property(legacy_level, "/display_name", "Legacy second");
        require(document.undo() && document.serialize() == legacy_first && document.can_redo(),
            "Legacy history fixture could not retain an edit branch");
        document.save(scratch / "legacy-history-upgrade.json");
        const std::string saved_revision = document.serialize();
        require(!document.dirty() && document.can_undo() && document.can_redo() &&
            parse(saved_revision)["format"].GetInt() == level_format::current,
            "Legacy save upgrade lost retained edit history");
        require(document.undo() && document.dirty() && document.redo() &&
            document.serialize() == saved_revision && !document.dirty(),
            "Legacy upgrade failed to mark just the saved history revision clean");
        require(document.redo() && document.dirty(), "Legacy upgrade discarded the redo branch");
        document.save(document.path());
        require(!document.dirty() && parse(document.serialize())["format"].GetInt() == level_format::current &&
            parse(document.serialize())["author_extension"] == legacy["author_extension"],
            "Legacy history save did not retain its upgraded version");
        std::cout << "PASS shared current format, legacy save upgrade, unknown fields, strict version gate and transactional history\n";
    }

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
        std::cout << "PASS ending-sting editor validation and gain boundaries\n";
    }

    void check_unknown_fields_and_rectangles(const fs::path& scratch)
    {
        LevelDocument seed;
        const Entry added = seed.add_object("Structure", { 10, 20, 80, 40 });
        std::string text = seed.serialize();
        text.insert(text.find('{') + 1,
            "\n\"author_extension\": {\"note\": \"keep me\", \"flags\": [1, true, null]},");
        const std::size_t geometry = text.find("\"rectangle\"", text.find("\"collision_objects\""));
        require(geometry != std::string::npos, "Template collision geometry absent");
        text.insert(geometry, "\"triangle\": {\"editor_hint\": true},\n");
        const fs::path input = scratch / "unknown-fields.json";
        write(input, text);
        LevelDocument document;
        document.load(input);
        const Entry target = entry(document, EntryKind::collision, added.index);
        const Rect moved{ 110, 220, 160, 70 };
        document.set_rectangle(target, moved);
        require(equal(document.rectangle(target), moved), "Rectangle edit produced wrong geometry");
        document.save(scratch / L"level-\u6d4b\u8bd5-\u00e9.json");
        LevelDocument reopened;
        reopened.load(document.path());
        const rapidjson::Document saved = parse(reopened.serialize());
        const rapidjson::Document input_json = parse(text);
        require(saved["author_extension"] == input_json["author_extension"],
            "Unknown nested root fields did not survive an edit/save");
        require(saved["collision_objects"][0]["triangle"]["editor_hint"].GetBool(),
            "Unknown triangle metadata on a rectangular object was lost");
        require(equal(reopened.rectangle(target), moved), "Unicode-path save lost geometry");

        // Replace just this known object's rectangle with the alternate encoding.
        std::string alternate = reopened.serialize();
        const std::string rectangle_key = "\"rectangle\"";
        const std::size_t object_start = alternate.find("\"collision_objects\"");
        const std::size_t rectangle_start = alternate.find(rectangle_key, object_start);
        require(rectangle_start != std::string::npos, "Template rectangle absent");
        const std::size_t rectangle_end = alternate.find('}', rectangle_start);
        alternate.replace(rectangle_start, rectangle_end - rectangle_start + 1,
            "\"rectangle_tlbr\": {\"left\": 12, \"top\": 34, \"right\": 112, \"bottom\": 94,"
            " \"author_geometry\": {\"preserve\": true}}");
        const fs::path alternate_path = scratch / "alternate-rectangle.json";
        write(alternate_path, alternate);
        document.load(alternate_path);
        const Entry first = entry(document, EntryKind::collision);
        require(equal(document.rectangle(first), { 12, 34, 100, 60 }),
            "rectangle_tlbr decoded incorrectly");
        document.set_rectangle(first, { 60, 80, 240, 120 });
        const rapidjson::Document edited = parse(document.serialize());
        const rapidjson::Value& object = edited["collision_objects"][0];
        require(!object.HasMember("rectangle"), "Editing changed rectangle encoding");
        const rapidjson::Value& rectangle = object["rectangle_tlbr"];
        require(rectangle["left"].GetDouble() == 60 && rectangle["top"].GetDouble() == 80 &&
            rectangle["right"].GetDouble() == 300 && rectangle["bottom"].GetDouble() == 200,
            "rectangle_tlbr edit wrote wrong corners");
        require(rectangle["author_geometry"]["preserve"].GetBool(),
            "Editing geometry lost unknown nested fields");
        std::cout << "PASS unknown fields, Unicode paths and both rectangle encodings\n";
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

    void check_palette_and_ramps(const fs::path& scratch)
    {
        LevelDocument document;
        require(document.dirty() && document.path().empty() && document.validate().empty(),
            "A new document must be valid and unsaved");
        for (const char* type : { "Structure", "StructureJumpThrough", "StructurePaintable",
            "StructureRampRight", "StructureRampLeft", "Visual", "SpawnA", "SpawnB" })
        {
            // Team B spawns on the right half of the template's stage.
            const double left = std::string(type) == "SpawnB" ? 2600 : 100;
            const Entry added = document.add_object(type, { left, 200, 300, 150 });
            require(document.validate().empty(), "A palette item creates an invalid document");
            if (std::string(type) == "StructureJumpThrough")
            {
                const rapidjson::Document json = parse(document.serialize());
                const rapidjson::Value& object = json["collision_objects"][static_cast<rapidjson::SizeType>(added.index)];
                require(std::string(object["type"].GetString()) == "Structure" &&
                    std::string(object["collision_type"].GetString()) == "STRUCTURE_JUMP_THROUGH",
                    "Jump-through template does not use the runtime object vocabulary");
            }
            if (std::string(type).find("Ramp") != std::string::npos)
            {
                const rapidjson::Document before = parse(document.serialize());
                const rapidjson::Value& original = before["collision_objects"][static_cast<rapidjson::SizeType>(added.index)]["triangle"];
                document.set_rectangle(added, { 140, 260, 600, 75 });
                require(equal(document.rectangle(added), { 140, 260, 600, 75 }),
                    "Ramp move/resize changed the wrong bounding box");
                const rapidjson::Document after = parse(document.serialize());
                const rapidjson::Value& transformed = after["collision_objects"][static_cast<rapidjson::SizeType>(added.index)]["triangle"];
                for (const char* axis : { "x1", "x2", "x3", "y1", "y2", "y3" })
                {
                    const bool x = axis[0] == 'x';
                    const double expected = x ? 140 + (original[axis].GetDouble() - 100) * 2 :
                        260 + (original[axis].GetDouble() - 200) * 0.5;
                    require(std::abs(transformed[axis].GetDouble() - expected) < 0.000001,
                        "Ramp transformation did not preserve its vertices");
                }
            }
            const std::string before_duplicate = document.serialize();
            const Entry copy = document.duplicate(added);
            require(document.entries().size() > 0 && document.validate().empty(),
                "Duplicating a palette item produced invalid data");
            document.erase(copy);
            require(parse(document.serialize()) == parse(before_duplicate),
                "Duplicate then erase did not restore source data");
        }
        document.save(scratch / "all-palette-items.json");
        LevelDocument reopened;
        reopened.load(document.path());
        require(parse(document.serialize()) == parse(reopened.serialize()),
            "Palette document did not survive saving");
        std::cout << "PASS palette runtime vocabulary, ramp transforms and duplicate/delete\n";
    }

    void check_history_and_failures(const fs::path& scratch)
    {
        LevelDocument document;
        const fs::path path = scratch / "history.json";
        document.save(path);
        const std::string saved = document.serialize();
        require(!document.dirty(), "Successful save did not mark document clean");
        const Entry level = entry(document, EntryKind::level);
        document.set_property(level, "/display_name", "Acceptance arena");
        require(document.dirty() && document.can_undo(), "Property edit did not enter history");
        const std::string edited = document.serialize();
        require(document.undo() && document.serialize() == saved && !document.dirty(),
            "Undo did not restore the saved clean state");
        require(document.redo() && document.serialize() == edited && document.dirty(),
            "Redo did not restore the property edit");
        document.save(path);
        require(!document.dirty(), "Second save left document dirty");
        require(document.undo() && document.dirty(), "Undo away from saved state must be dirty");
        require(document.redo() && !document.dirty(), "Redo back to saved state must be clean");
        require(document.undo(), "Expected an undoable edit");
        document.set_property(level, "/display_name", "Branched arena");
        require(!document.can_redo(), "New edit failed to discard the redo branch");
        reject_without_change(document, [&] {
            document.set_property(level, "/display_name", "Arena \xF0\x9F\x8E\xA8");
        }, "Stage name with unsupported font glyph was accepted");
        document.set_property(level, "/display_name", "Arena \xE2\x80\x93 smart punctuation");
        require(document.validate().empty(), "Runtime-normalized punctuation was rejected");

        const Entry object = document.add_object("Structure", { 100, 100, 200, 100 });
        document.set_property(object, "/rectangle/width", "325.5");
        require(equal(document.rectangle(object), { 100, 100, 325.5, 100 }),
            "Valid decimal property edit did not change geometry");
        const Entry paintable = document.add_object("StructurePaintable", { 500, 100, 200, 100 });
        document.set_property(paintable, "/paintable_faces/top", "false");
        const rapidjson::Document with_face_edit = parse(document.serialize());
        require(!with_face_edit["collision_objects"][static_cast<rapidjson::SizeType>(paintable.index)]
            ["paintable_faces"]["top"].GetBool(), "Boolean inspector edit did not change a paintable face");
        reject_without_change(document, [&] { document.set_property(paintable, "/paintable_faces/top", "yes"); },
            "Invalid boolean property was accepted");
        reject_without_change(document, [&] { document.set_rectangle(object, { 0, 0, -1, 100 }); },
            "Negative rectangle size was accepted");
        reject_without_change(document, [&] { document.set_rectangle(object, { 0, 0, 1e-300, 100 }); },
            "Rectangle width which underflows the game's float was accepted");
        reject_without_change(document, [&] {
            document.set_rectangle(object, { std::numeric_limits<double>::infinity(), 0, 10, 10 });
        }, "Non-finite geometry was accepted");
        reject_without_change(document, [&] { document.set_property(object, "/rectangle/width", "oops"); },
            "Invalid numeric property was accepted");
        reject_without_change(document, [&] { document.set_property(object, "/type", "NotAnObject"); },
            "Unknown runtime object type was accepted");
        const Entry spawn = entry(document, EntryKind::spawn_a);
        reject_without_change(document, [&] { document.erase(spawn); },
            "Deleting the fourth required spawn was accepted");
        reject_without_change(document, [&] { document.set_rectangle(spawn, { 1e12, 0, 52, 120 }); },
            "Out-of-bounds spawn was accepted");
        reject_without_change(document, [&] { document.erase(level); }, "Deleting level root was accepted");
        reject_without_change(document, [&] { document.add_object("NotAnObject", { 0, 0, 10, 10 }); },
            "Unknown palette type was accepted");
        reject_without_change(document, [&] { document.load(scratch / "does-not-exist.json"); },
            "Loading absent file succeeded");
        write(scratch / "malformed.json", "{broken");
        reject_without_change(document, [&] { document.load(scratch / "malformed.json"); },
            "Malformed JSON was accepted");
        write(scratch / "invalid-schema.json", "{}");
        reject_without_change(document, [&] { document.load(scratch / "invalid-schema.json"); },
            "Missing required fields were accepted");

        const std::string saved_bytes = contents(path);
        const HANDLE locked_file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(locked_file != INVALID_HANDLE_VALUE, "Could not lock destination for write-failure check");
        try
        {
            reject_without_change(document, [&] { document.save(path); },
                "Saving over a file without delete sharing succeeded");
            require(contents(path) == saved_bytes, "Failed replacement changed the existing saved file");
        }
        catch (...)
        {
            CloseHandle(locked_file);
            throw;
        }
        CloseHandle(locked_file);
        const DWORD attributes = GetFileAttributesW(path.c_str());
        require(attributes != INVALID_FILE_ATTRIBUTES &&
            SetFileAttributesW(path.c_str(), attributes | FILE_ATTRIBUTE_READONLY),
            "Could not mark destination read-only for save-failure check");
        try
        {
            reject_without_change(document, [&] { document.save(path); },
                "Saving over a read-only file succeeded");
            require(contents(path) == saved_bytes, "Rejected read-only save changed the original bytes");
        }
        catch (...)
        {
            SetFileAttributesW(path.c_str(), attributes);
            throw;
        }
        require(SetFileAttributesW(path.c_str(), attributes) != 0,
            "Could not restore destination attributes after save-failure check");
        const fs::path blocked = scratch / "blocked.json";
        fs::create_directory(blocked);
        write(blocked / "sentinel.txt", "retain existing directory");
        reject_without_change(document, [&] { document.save(blocked); },
            "Saving over an existing directory succeeded");
        require(contents(blocked / "sentinel.txt") == "retain existing directory" &&
            contents(path) == saved_bytes, "Failed save damaged existing filesystem data");
        for (const fs::directory_entry& item : fs::directory_iterator(scratch))
            require(item.path().filename().wstring().find(L".editor-") == std::wstring::npos,
                "A rejected save left a temporary or backup behind");
        document.save(path);
        require(!document.dirty(), "Saving after write failure did not recover");
        document.create();
        require(document.dirty() && document.path().empty() && !document.can_undo() &&
            !document.can_redo() && document.validate().empty(), "New failed to reset history/path");
        std::cout << "PASS saved-state history, branching and transactional failure recovery\n";
    }
}

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        require(argc == 3, "Usage: colourwars_editor_acceptance <repository> <empty-scratch-directory>");
        const fs::path repo = fs::absolute(argv[1]);
        const fs::path scratch = fs::absolute(argv[2]);
        require(fs::is_directory(scratch), "Acceptance scratch directory must already exist");
        check_shipped_levels(repo, scratch);
        check_format_versions(scratch);
        check_ending_audio(repo);
        check_blockout_aids(repo, scratch);
        check_unknown_fields_and_rectangles(scratch);
        check_palette_and_ramps(scratch);
        check_random_setting(scratch);
        check_history_and_failures(scratch);
        std::cout << "PASS level editor document acceptance\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL level editor acceptance: " << error.what() << '\n';
        return 1;
    }
}
