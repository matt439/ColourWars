// Editing and history checks: palette and ramps, and history across failed
// operations.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <Windows.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace level_editor_acceptance
{
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
