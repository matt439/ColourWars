// Format and round-trip checks: shipped levels, format versions, unknown
// fields and rectangle encodings.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/blockout_aids.h"
#include "game/level_format.h"
#include <Windows.h>

#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <utility>

namespace level_editor_acceptance
{
    void check_shipped_levels(const fs::path& repo, const fs::path& scratch)
    {
        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters",
            "movement_course", "crossfeed", "press_tower", "turbulence_blockout" })
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
                {"close_quarters", 136320.0}, {"movement_course", 202080.0}, {"crossfeed", 124320.0},
                {"press_tower", 140400.0}, {"turbulence_blockout", 148800.0}};
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

        // A zero source byte is not JSON's escaped \u0000. A valid prefix must
        // not make Load silently accept (and later save away) a file suffix.
        LevelDocument raw_nul;
        std::string escaped_source = raw_nul.serialize();
        escaped_source.insert(escaped_source.find('{') + 1, R"("nul_positive":"keep\u0000tail",)");
        const fs::path raw_baseline = scratch / "raw-nul-live.json";
        write(raw_baseline, escaped_source);
        raw_nul.load(raw_baseline);
        const std::string initial_raw = raw_nul.serialize();
        const Entry raw_level = entry(raw_nul, EntryKind::level);
        bool escaped_offered = false;
        for (const Property& property : raw_nul.properties(raw_level))
            escaped_offered |= property.path == "nul_positive" &&
                property.value == std::string("keep") + '\0' + "tail";
        require(escaped_offered && initial_raw.find('\0') == std::string::npos,
            "Escaped JSON NUL must remain valid metadata without a raw source zero");
        raw_nul.set_property(raw_level, "/display_name", "Raw NUL first");
        const std::string raw_first = raw_nul.serialize();
        raw_nul.set_property(raw_level, "/display_name", "Raw NUL second");
        const std::string raw_second = raw_nul.serialize();
        require(raw_nul.undo() && raw_nul.can_undo() && raw_nul.can_redo() && raw_nul.dirty(),
            "Raw-NUL refusal fixture needs a complete dirty undo/redo branch");
        const auto full_raw_history = [&]
        {
            require(raw_nul.path() == raw_baseline && raw_nul.serialize() == raw_first && raw_nul.dirty() &&
                raw_nul.can_undo() && raw_nul.can_redo() &&
                raw_nul.undo() && raw_nul.serialize() == initial_raw && !raw_nul.dirty() &&
                !raw_nul.can_undo() && raw_nul.can_redo() &&
                raw_nul.redo() && raw_nul.serialize() == raw_first && raw_nul.dirty() &&
                raw_nul.redo() && raw_nul.serialize() == raw_second && raw_nul.dirty() && !raw_nul.can_redo() &&
                raw_nul.undo() && raw_nul.serialize() == raw_first && raw_nul.can_undo() && raw_nul.can_redo(),
                "A refused raw-NUL load changed exact history snapshots, saved state or path");
        };
        const std::string zero(1, '\0');
        const std::array<std::pair<const char*, std::string>, 5> suffixes{ {
            { "garbage", zero + "discarded source bytes" },
            { "bare", zero },
            { "whitespace-before", std::string(" \t\r\n") + zero },
            { "whitespace-after", zero + " \t\r\n" },
            { "second-root", zero + R"({"author_note":"must not disappear"})" }
        } };
        for (bool bom : { false, true })
        {
            const std::string prefix = bom ? "\xEF\xBB\xBF" : "";
            for (const auto& [name, suffix] : suffixes)
            {
                const fs::path invalid = scratch / (std::string(bom ? "raw-nul-bom-" : "raw-nul-") + name + ".json");
                const std::string bytes = prefix + initial_raw + suffix;
                write(invalid, bytes);
                std::string diagnostic;
                reject_without_change(raw_nul, [&]
                {
                    try { raw_nul.load(invalid); }
                    catch (const std::exception& error) { diagnostic = error.what(); throw; }
                }, "Load accepted a valid JSON prefix followed by raw NUL source bytes");
                // Existing parser offsets exclude an optional UTF-8 BOM.
                require(diagnostic.find(invalid.filename().string()) != std::string::npos &&
                    diagnostic.find("Invalid JSON at byte " + std::to_string(bytes.find('\0') - prefix.size()) + ":") != std::string::npos,
                    "Raw-NUL load refusal must identify its file and BOM-stripped zero-byte offset");
                require(contents(invalid) == bytes && contents(raw_baseline) == escaped_source,
                    "A refused raw-NUL load changed either the invalid source or the open saved file");
                full_raw_history();
            }
            const fs::path malformed_nul = scratch /
                (bom ? "raw-nul-bom-malformed-first.json" : "raw-nul-malformed-first.json");
            const std::string malformed_bytes = prefix + "{broken" + zero + "later suffix";
            write(malformed_nul, malformed_bytes);
            std::string malformed_diagnostic;
            reject_without_change(raw_nul, [&]
            {
                try { raw_nul.load(malformed_nul); }
                catch (const std::exception& error) { malformed_diagnostic = error.what(); throw; }
            }, "Malformed JSON with a later raw NUL was accepted");
            require(malformed_diagnostic.find(malformed_nul.filename().string()) != std::string::npos &&
                malformed_diagnostic.find("Invalid JSON at byte 1: Missing a name for object member.") != std::string::npos,
                "A later raw NUL replaced the existing malformed-JSON first diagnostic");
            require(contents(malformed_nul) == malformed_bytes && contents(raw_baseline) == escaped_source,
                "Malformed-JSON refusal changed source bytes");
            full_raw_history();
        }
        raw_nul.save(raw_baseline);
        require(contents(raw_baseline) == raw_first && !raw_nul.dirty() &&
            raw_nul.can_undo() && raw_nul.can_redo(),
            "Refused raw-NUL loads damaged the original save baseline or retained history");
        LevelDocument raw_reopened;
        raw_reopened.load(raw_baseline);
        require(raw_reopened.serialize() == raw_first,
            "Saving after raw-NUL refusals lost escaped metadata or the live document");
        const fs::path whitespace_path = scratch / "raw-nul-valid-whitespace.json";
        const std::string whitespace_bytes = initial_raw + " \t\r\n";
        write(whitespace_path, whitespace_bytes);
        LevelDocument whitespace_document;
        whitespace_document.load(whitespace_path);
        require(whitespace_document.serialize() == initial_raw && !whitespace_document.dirty() &&
            !whitespace_document.can_undo() && !whitespace_document.can_redo() &&
            contents(whitespace_path) == whitespace_bytes,
            "Ordinary trailing JSON whitespace or escaped NUL metadata was refused or changed");
        std::cout << "PASS raw-NUL source refusal: complete suffix admission, byte offsets, first parser errors, "
            "source bytes and full history; escaped NUL and ordinary trailing whitespace preserved\n";
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

        // Unknown JSON keys are byte strings, including NUL and JSON Pointer's
        // escaped '~' and '/'. Edit the actual inspector path, not its prefix.
        auto nul_metadata = parse(reopened.serialize());
        const std::string prefix_key = "note~/";
        const std::string nul_key("note~/\0tail", 11);
        const std::string original_value("original\0tail", 13);
        const std::string changed_value("changed\0tail", 12);
        const std::string nul_name("named\0tail", 10);
        auto& extension = nul_metadata["author_extension"];
        extension.AddMember(rapidjson::Value(prefix_key.data(),
            static_cast<rapidjson::SizeType>(prefix_key.size()), nul_metadata.GetAllocator()),
            rapidjson::Value("prefix sibling", nul_metadata.GetAllocator()), nul_metadata.GetAllocator());
        extension.AddMember(rapidjson::Value(nul_key.data(),
            static_cast<rapidjson::SizeType>(nul_key.size()), nul_metadata.GetAllocator()),
            rapidjson::Value(original_value.data(), static_cast<rapidjson::SizeType>(original_value.size()),
                nul_metadata.GetAllocator()), nul_metadata.GetAllocator());
        nul_metadata["collision_objects"][static_cast<rapidjson::SizeType>(target.index)]["name"].SetString(
            nul_name.data(), static_cast<rapidjson::SizeType>(nul_name.size()), nul_metadata.GetAllocator());
        const fs::path nul_path = scratch / "unknown-nul-metadata.json";
        const std::string nul_bytes = encode(nul_metadata);
        write(nul_path, nul_bytes);
        LevelDocument nul_document;
        nul_document.load(nul_path);
        const Entry nul_level = entry(nul_document, EntryKind::level);
        std::string full_path = "author_extension/note~0~1";
        full_path.append("\0tail", 5);
        bool offered = false;
        for (const Property& property : nul_document.properties(nul_level))
        {
            offered |= property.path == full_path && property.value == original_value;
        }
        require(offered, "Inspector lost the complete NUL-bearing unknown property path or value");
        nul_document.set_property(nul_level, full_path, changed_value);
        const auto changed_nul = parse(nul_document.serialize());
        const auto& changed_extension = changed_nul["author_extension"];
        const rapidjson::Value full_key(rapidjson::StringRef(nul_key.data(),
            static_cast<rapidjson::SizeType>(nul_key.size())));
        const auto full_member = changed_extension.FindMember(full_key);
        require(full_member != changed_extension.MemberEnd() && full_member->value.IsString() &&
            std::string(full_member->value.GetString(), full_member->value.GetStringLength()) == changed_value &&
            changed_extension[prefix_key.c_str()].IsString() &&
            std::string(changed_extension[prefix_key.c_str()].GetString(),
                changed_extension[prefix_key.c_str()].GetStringLength()) == "prefix sibling",
            "NUL-bearing property path edited its prefix sibling or failed to preserve the complete key/value");
        const std::string changed_nul_bytes = nul_document.serialize();
        const fs::path nul_roundtrip = scratch / "unknown-nul-roundtrip.json";
        nul_document.save(nul_roundtrip);
        LevelDocument nul_reopened;
        nul_reopened.load(nul_roundtrip);
        require(parse(nul_reopened.serialize()) == changed_nul && !nul_reopened.dirty() &&
            contents(nul_path) == nul_bytes && nul_document.path() == nul_roundtrip &&
            nul_document.undo() && parse(nul_document.serialize()) == nul_metadata &&
            nul_document.redo() && nul_document.serialize() == changed_nul_bytes,
            "NUL-bearing unknown keys/values lost bytes through save/reopen or undo/redo");

        const Entry named_object = entry(nul_document, EntryKind::collision, target.index);
        const std::string before_duplicate = nul_document.serialize();
        const Entry copied = nul_document.duplicate(named_object);
        const auto duplicated = parse(nul_document.serialize());
        const auto& original_name = duplicated["collision_objects"]
            [static_cast<rapidjson::SizeType>(named_object.index)]["name"];
        const auto& copied_name = duplicated["collision_objects"]
            [static_cast<rapidjson::SizeType>(copied.index)]["name"];
        const std::string expected_copy = nul_name + " copy";
        require(original_name.IsString() && copied_name.IsString() &&
            std::string(original_name.GetString(), original_name.GetStringLength()) == nul_name &&
            copied_name.GetStringLength() == expected_copy.size() &&
            std::string(copied_name.GetString(), copied_name.GetStringLength()) == expected_copy &&
            copied.label == expected_copy,
            "Duplicating an object truncated its NUL-bearing name or the appended copy suffix");
        const std::string after_duplicate = nul_document.serialize();
        require(nul_document.undo() && nul_document.serialize() == before_duplicate &&
            nul_document.redo() && nul_document.serialize() == after_duplicate &&
            nul_document.path() == nul_roundtrip,
            "NUL-bearing duplicate names did not survive exact undo/redo snapshots");

        // These known enum paths insert missing strings. They must validate
        // the complete input rather than accept a valid prefix before NUL.
        LevelDocument missing_material;
        const Entry paintable = missing_material.add_object("StructurePaintable", { 600, 200, 80, 40 });
        missing_material.set_property(paintable, "/material", "");
        missing_material.save(scratch / "nul-missing-material.json");
        const Entry material_level = entry(missing_material, EntryKind::level);
        missing_material.set_property(material_level, "/display_name", "Material first");
        const std::string material_first = missing_material.serialize();
        missing_material.set_property(material_level, "/display_name", "Material second");
        const std::string material_second = missing_material.serialize();
        require(missing_material.undo() && missing_material.can_undo() && missing_material.can_redo(),
            "NUL-bearing material refusal fixture needs both undo and redo history");
        require(!parse(missing_material.serialize())["collision_objects"]
            [static_cast<rapidjson::SizeType>(paintable.index)].HasMember("material"),
            "Material refusal fixture did not exercise missing-field insertion");
        reject_without_change(missing_material, [&]
        {
            missing_material.set_property(paintable, "/material", std::string("pencil\0invalid", 14));
        }, "Missing material accepted an invalid NUL-bearing enum suffix");
        require(missing_material.redo() && missing_material.serialize() == material_second &&
            missing_material.undo() && missing_material.serialize() == material_first,
            "Refused NUL-bearing material changed undo/redo snapshots");

        const fs::path nul_tuning = scratch / "nul-motion-tuning.json";
        write(nul_tuning, R"({"player":{"size":{"x":62,"y":100}}})");
        LevelDocument missing_drive(nul_tuning);
        const Entry moving = missing_drive.add_object("MovingPaintable", { 600, 600, 320, 20 });
        missing_drive.save(scratch / "nul-missing-drive.json");
        const Entry drive_level = entry(missing_drive, EntryKind::level);
        missing_drive.set_property(drive_level, "/display_name", "Drive first");
        const std::string drive_first = missing_drive.serialize();
        missing_drive.set_property(drive_level, "/display_name", "Drive second");
        const std::string drive_second = missing_drive.serialize();
        require(missing_drive.undo() && missing_drive.can_undo() && missing_drive.can_redo() &&
            !parse(missing_drive.serialize())["collision_objects"]
                [static_cast<rapidjson::SizeType>(moving.index)]["motion"].HasMember("drive"),
            "NUL-bearing drive refusal fixture needs a missing drive and both history branches");
        reject_without_change(missing_drive, [&]
        {
            missing_drive.set_property(moving, "/motion/drive", std::string("cycle\0invalid", 13));
        }, "Missing motion drive accepted an invalid NUL-bearing enum suffix");
        require(missing_drive.redo() && missing_drive.serialize() == drive_second &&
            missing_drive.undo() && missing_drive.serialize() == drive_first,
            "Refused NUL-bearing motion drive changed undo/redo snapshots");
        missing_drive.set_property(moving, "/motion/drive", "paint");
        reject_without_change(missing_drive, [&]
        {
            missing_drive.set_property(moving, "/motion/team_a_heads", std::string("right\0invalid", 13));
        }, "Existing paint heading accepted an invalid NUL-bearing enum suffix");
        std::cout << "PASS NUL-bearing unknown metadata edits, duplicate names and complete enum validation; "
            "exact bytes, private paths and undo/redo preserved\n";
        std::cout << "PASS unknown fields, Unicode paths and both rectangle encodings\n";
    }
}
