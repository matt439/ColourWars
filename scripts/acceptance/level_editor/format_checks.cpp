// Format and round-trip checks: shipped levels, format versions, unknown
// fields and rectangle encodings.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/blockout_aids.h"
#include "game/level_format.h"
#include <Windows.h>

#include <cmath>
#include <exception>
#include <iostream>
#include <map>
#include <string>

namespace level_editor_acceptance
{
    void check_shipped_levels(const fs::path& repo, const fs::path& scratch)
    {
        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters",
            "movement_course", "crossfeed" })
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
                {"close_quarters", 136320.0}, {"movement_course", 194880.0}, {"crossfeed", 38880.0}};
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
}
