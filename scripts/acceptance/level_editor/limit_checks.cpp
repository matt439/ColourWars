// The format's upper limits (level_format::limit, REV#2): the editor refuses
// at load, on an edit and on save what the game refuses at load.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "game/contracts/level_format.h"

#include <functional>
#include <iostream>
#include <string>

namespace level_editor_acceptance
{
    void check_format_limits(const fs::path& scratch)
    {
        namespace limit = level_format::limit;
        LevelDocument document;
        const std::string original = document.serialize();
        const auto variant = [&](const char* name, const std::function<void(rapidjson::Document&)>& edit)
        {
            auto json = parse(original);
            edit(json);
            const fs::path path = scratch / (std::string("limit-") + name + ".json");
            write_text(path, encode(json));
            return path;
        };
        const std::string coordinate = "coordinate limit of +/-" + limit::whole(limit::max_coordinate);
        const std::string extent = "wider or taller than the level format's limit of " +
            limit::whole(limit::max_object_extent);
        const std::string paint = limit::whole(limit::max_paint_length) + " world units of paint";
        const double long_face = (limit::max_paint_length + limit::max_object_extent) / 2.0;

        // File > New's left wall is collision_objects[0] and its paintable floor [3].
        refuses_file(document, variant("wide-bounds", [](rapidjson::Document& d)
            { d["level_bounds"]["out_of_bounds"]["width"].SetDouble(60000000.0); }),
            "out_of_bounds has an edge outside the level format's " + coordinate, "Over-wide bounds opened");
        refuses_file(document, variant("wide-object", [](rapidjson::Document& d)
            {
                d["collision_objects"][0]["rectangle"]["x"].SetDouble(-limit::max_coordinate + 1000.0);
                d["collision_objects"][0]["rectangle"]["width"].SetDouble(limit::max_object_extent + 500.0);
            }), extent, "An over-size object opened");
        refuses_file(document, variant("far-object", [](rapidjson::Document& d)
            { d["collision_objects"][0]["rectangle"]["x"].SetDouble(1.0e9); }), coordinate,
            "An object far from the origin opened");
        refuses_file(document, variant("far-edge", [](rapidjson::Document& d)
            {
                d["collision_objects"][0]["rectangle"]["x"].SetDouble(limit::max_coordinate - 50.0);
                d["collision_objects"][0]["rectangle"]["width"].SetDouble(100.0);
            }), coordinate, "An object whose right edge passes the limit opened");
        refuses_file(document, variant("many-objects", [](rapidjson::Document& d)
            {
                rapidjson::Value& visuals = d["non_collision_objects"];
                while (visuals.Size() <= limit::max_objects)
                {
                    rapidjson::Value copy(rapidjson::kObjectType);
                    copy.CopyFrom(visuals[0], d.GetAllocator());
                    visuals.PushBack(copy, d.GetAllocator());
                }
            }), "non_collision_objects holds " + std::to_string(limit::max_objects + 1) + " objects",
            "A level with too many objects opened");
        refuses_file(document, variant("long-paint", [&](rapidjson::Document& d)
            {
                d["collision_objects"][3]["rectangle"]["x"].SetDouble(-long_face / 2.0);
                d["collision_objects"][3]["rectangle"]["width"].SetDouble(long_face);
            }), paint, "A level with too much paintable face opened");

        // One byte over the file limit is refused; exactly the limit opens.
        require(original.size() < limit::max_file_bytes, "The template must be inside the file limit");
        const fs::path over = scratch / "limit-file-over.json";
        write_text(over, original + std::string(limit::max_file_bytes + 1 - original.size(), ' '));
        refuses_file(document, over, std::to_string(limit::max_file_bytes) + " bytes for one level file",
            "A level file over the size limit opened");
        const fs::path at = scratch / "limit-file-at.json";
        write_text(at, original + std::string(limit::max_file_bytes - original.size(), ' '));
        LevelDocument largest;
        largest.load(at);
        require(parse(largest.serialize()) == parse(original), "A level file at the size limit did not open intact");

        // An edit past a limit is refused and changes nothing.
        const Entry floor_entry = named(document, EntryKind::collision, "paintable_floor");
        reject_without_change(document,
            [&] { document.set_rectangle(floor_entry, { -long_face / 2.0, 1200, long_face, 100 }); },
            "An edit past the paint limit was applied");
        reject_without_change(document, [&] { document.set_rectangle(floor_entry, { 1.0e9, 1200, 3000, 100 }); },
            "An edit past the coordinate limit was applied");
        reject_without_change(document, [&]
            { document.set_rectangle(floor_entry, { -limit::max_coordinate + 1000.0, 1200, limit::max_object_extent + 500.0, 100 }); },
            "An edit past the object size limit was applied");

        // A document whose text is over the file limit is not written.
        LevelDocument wordy;
        wordy.set_property(entry(wordy, EntryKind::level), "display_name", std::string(limit::max_file_bytes, 'a'));
        const fs::path unsaved = scratch / "limit-unsaved.json";
        refuses([&] { wordy.save(unsaved); }, std::to_string(limit::max_file_bytes) + " bytes for one level file",
            "A level over the file limit was saved");
        require(!fs::exists(unsaved) && wordy.path().empty(), "A refused over-size save wrote or adopted its destination");
        std::cout << "PASS format limits: over-wide bounds, over-size and far objects, too many objects, too much paint "
            "and an over-size file refused at load; edits past a limit change nothing; an over-size save writes nothing\n";
    }
}
