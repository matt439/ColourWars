// Generated inspector paths must retain empty JSON Pointer tokens. Every
// authored input and output below belongs to the caller's private scratch.
#include "scripts/acceptance/level_editor/fixtures.h"

#include <iostream>
#include <utility>
#include <vector>

namespace level_editor_acceptance
{
    namespace
    {
        std::string text(const rapidjson::Value& value)
        {
            require(value.IsString(), "ED16 fixture value is not text");
            return { value.GetString(), value.GetStringLength() };
        }

        Property offered_value(const LevelDocument& document, const Entry& selected,
            const std::string& value)
        {
            Property found;
            std::size_t count = 0;
            for (const Property& property : document.properties(selected))
            {
                if (property.value == value) { found = property; ++count; }
            }
            require(count == 1, "ED16 needs one generated property for " + value);
            return found;
        }

        void put_object(rapidjson::Value& target, const char* name, const char* json,
            rapidjson::Document::AllocatorType& allocator)
        {
            const auto source = parse(json);
            rapidjson::Value value;
            value.CopyFrom(source, allocator);
            target.AddMember(rapidjson::Value(name, allocator), value, allocator);
        }
    }

    void check_empty_property_keys(const fs::path& scratch)
    {
        LevelDocument seed;
        const Entry shadow = seed.add_object("Structure", { 300, 200, 160, 80 });
        const Entry scalar = seed.add_object("Structure", { 600, 200, 160, 80 });
        auto fixture = parse(seed.serialize());
        auto& allocator = fixture.GetAllocator();
        put_object(fixture, "", R"({
            "display_name":"ED16 nested stage name",
            "":{"leaf":"ED16 multiple empty tokens"},
            "a/b~":"ED16 escaped leaf",
            "a~1b~0":"ED16 literal escape sibling",
            "kept_array":[{"":"ED16 array key","type":"StructureRamp"},7,true,null]
        })", allocator);
        auto& shadow_json = fixture["collision_objects"][static_cast<rapidjson::SizeType>(shadow.index)];
        put_object(shadow_json, "", R"({
            "type":"ED16 shadow type",
            "rectangle":{"width":17.25},
            "":"ED16 nested object empty leaf"
        })", allocator);
        fixture["collision_objects"][static_cast<rapidjson::SizeType>(scalar.index)].AddMember(
            "", rapidjson::Value("ED16 object empty scalar", allocator), allocator);
        const fs::path source = scratch / "empty-key-source.json";
        const std::string source_bytes = " \n" + encode(fixture) + "\n ";
        write(source, source_bytes);

        LevelDocument document;
        document.load(source);
        const Entry level = entry(document, EntryKind::level);
        const Entry object = entry(document, EntryKind::collision, shadow.index);
        const Entry scalar_object = entry(document, EntryKind::collision, scalar.index);
        const std::string original_name = text(fixture["display_name"]);
        std::vector<std::string> snapshots{ document.serialize() };
        require(document.path() == source && !document.dirty() && !document.can_undo() &&
            !document.can_redo(), "ED16 load did not establish a clean private source");

        // First new regression: use the actual offered row without requiring a
        // spelling first. The old path changes the real stage name instead.
        const Property nested_name = offered_value(document, level, "ED16 nested stage name");
        document.set_property(level, nested_name.path, "ED16 edited nested stage name");
        const auto first = parse(document.serialize());
        require(text(first[""]["display_name"]) == "ED16 edited nested stage name" &&
            text(first["display_name"]) == original_name,
            "ED16 generated empty-key property edited the real stage name instead of its nested metadata");
        require(nested_name.path == "//display_name",
            "ED16 nested empty-key path did not retain its first empty token");
        snapshots.push_back(document.serialize());

        const auto edit_row = [&](const Entry& selected, const std::string& old_value,
            const std::string& expected_path, const std::string& new_value)
        {
            const Property property = offered_value(document, selected, old_value);
            require(property.path == expected_path, "ED16 ambiguous generated path: " + property.path);
            document.set_property(selected, property.path, new_value);
            snapshots.push_back(document.serialize());
        };
        edit_row(level, "ED16 multiple empty tokens", "///leaf", "ED16 edited deep leaf");
        edit_row(level, "ED16 escaped leaf", "//a~1b~0", "ED16 edited escaped leaf");
        edit_row(object, "ED16 shadow type", "//type", "ED16 edited shadow type");
        edit_row(object, "17.25", "//rectangle/width", "18.5");
        edit_row(object, "ED16 nested object empty leaf", "//", "ED16 edited nested object leaf");
        edit_row(scalar_object, "ED16 object empty scalar", "/", "ED16 edited object scalar");

        // Both public setter spellings remain accepted for ordinary fields.
        document.set_property(level, "display_name", "ED16 ordinary bare name");
        snapshots.push_back(document.serialize());
        document.set_property(level, "/display_name", original_name);
        snapshots.push_back(document.serialize());
        const auto edited = parse(document.serialize());
        const auto& edited_object = edited["collision_objects"][static_cast<rapidjson::SizeType>(shadow.index)];
        require(text(edited[""][""]["leaf"]) == "ED16 edited deep leaf" &&
            text(edited[""]["a/b~"]) == "ED16 edited escaped leaf" &&
            text(edited[""]["a~1b~0"]) == "ED16 literal escape sibling" &&
            edited[""]["kept_array"] == fixture[""]["kept_array"],
            "ED16 empty tokens or escaped names changed another field or array metadata");
        require(text(edited_object["type"]) == "Structure" &&
            edited_object["rectangle"] == shadow_json["rectangle"] &&
            text(edited_object[""]["type"]) == "ED16 edited shadow type" &&
            edited_object[""]["rectangle"]["width"].GetDouble() == 18.5 &&
            text(edited_object[""][""]) == "ED16 edited nested object leaf" &&
            text(edited["collision_objects"][static_cast<rapidjson::SizeType>(scalar.index)][""]) ==
                "ED16 edited object scalar",
            "ED16 unknown object metadata changed runtime type/geometry or another empty-key value");
        for (const Property& property : document.properties(level))
        {
            require(property.value != "ED16 array key",
                "ED16 path repair unexpectedly enabled array element editing");
        }
        require(document.path() == source && contents(source) == source_bytes &&
            document.dirty() && document.can_undo() && !document.can_redo(),
            "ED16 model edits changed the source file/path or failed to enter history");

        // Walk every snapshot in both directions, checking dirty state as well
        // as complete serialized metadata rather than only history booleans.
        for (std::size_t index = snapshots.size() - 1; index > 0; --index)
        {
            require(document.undo() && document.serialize() == snapshots[index - 1] &&
                document.dirty() == (index > 1) && document.path() == source &&
                contents(source) == source_bytes, "ED16 undo changed metadata/history/source state");
        }
        require(!document.can_undo() && document.can_redo(), "ED16 undo did not reach the load state");
        for (std::size_t index = 1; index < snapshots.size(); ++index)
        {
            require(document.redo() && document.serialize() == snapshots[index] &&
                document.dirty() && document.path() == source && contents(source) == source_bytes,
                "ED16 redo changed metadata/history/source state");
        }
        require(!document.can_redo(), "ED16 redo retained an unexpected extra edit");
        const fs::path output = scratch / "empty-key-roundtrip.json";
        document.save(output);
        LevelDocument reopened;
        reopened.load(output);
        require(document.path() == output && !document.dirty() &&
            document.serialize() == snapshots.back() && reopened.serialize() == snapshots.back() &&
            !reopened.dirty() && !reopened.can_undo() && !reopened.can_redo() &&
            contents(source) == source_bytes && parse(contents(output)) == edited,
            "ED16 save/reopen changed complete metadata, source bytes, path or clean state");
        require(document.undo() && document.serialize() == snapshots[snapshots.size() - 2] &&
            document.dirty() && document.redo() && document.serialize() == snapshots.back() &&
            !document.dirty(), "ED16 save/reopen discarded or rewrote the editor's history");

        // A scalar empty root key has no child separator; its generated '/'
        // pointer still means the empty member, never the whole document.
        fixture.RemoveMember("");
        fixture.AddMember("", rapidjson::Value("ED16 root empty scalar", allocator), allocator);
        const fs::path scalar_source = scratch / "empty-key-scalar-source.json";
        const std::string scalar_bytes = encode(fixture);
        write(scalar_source, scalar_bytes);
        LevelDocument scalar_document;
        scalar_document.load(scalar_source);
        const Entry scalar_level = entry(scalar_document, EntryKind::level);
        const Property scalar_row = offered_value(scalar_document, scalar_level, "ED16 root empty scalar");
        require(scalar_row.path == "/", "ED16 empty root scalar has an ambiguous generated path");
        scalar_document.set_property(scalar_level, scalar_row.path, "ED16 edited root scalar");
        const auto scalar_edited = parse(scalar_document.serialize());
        require(text(scalar_edited[""]) == "ED16 edited root scalar" &&
            text(scalar_edited["display_name"]) == original_name && contents(scalar_source) == scalar_bytes,
            "ED16 empty root scalar edit changed another field or private source");
        scalar_document.save(scratch / "empty-key-scalar-roundtrip.json");
        LevelDocument scalar_reopened;
        scalar_reopened.load(scalar_document.path());
        require(scalar_reopened.serialize() == scalar_document.serialize() && !scalar_reopened.dirty(),
            "ED16 empty root scalar failed private save/reopen");
        std::cout << "PASS empty JSON keys: generated root/object paths, nested empty tokens, escaped names, "
            "known-field shadows, scalar leaves, bare/absolute setters, arrays, full history and private save/reopen\n";
    }
}
