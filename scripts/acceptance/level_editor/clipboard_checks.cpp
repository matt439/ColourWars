// Copy and paste between documents (ED#27): copy_text puts one object, spawn
// or station in clipboard JSON, and paste_text appends it to another open
// level as one validated undo step. Malformed text, another shape and an
// object the receiving level would refuse leave that level untouched.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <functional>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace level_editor_acceptance
{
    namespace
    {
        void refuses(const std::function<void()>& action, const std::string& expected, const char* message)
        {
            bool rejected = false;
            try { action(); }
            catch (const std::exception& error)
            {
                rejected = true;
                require(std::string(error.what()).find(expected) != std::string::npos,
                    std::string(message) + ": diagnostic was '" + error.what() + "'");
            }
            require(rejected, message);
        }

        Entry last(const LevelDocument& document, EntryKind kind)
        {
            std::optional<Entry> found;
            for (const Entry& candidate : document.entries())
            {
                if (candidate.kind == kind) { found = candidate; }
            }
            require(found.has_value(), "The document lists no entry of the expected kind");
            return *found;
        }

        const rapidjson::Value& list(const rapidjson::Document& document, EntryKind kind)
        {
            switch (kind)
            {
            case EntryKind::collision: return document["collision_objects"];
            case EntryKind::decoration: return document["non_collision_objects"];
            case EntryKind::spawn_a: return document["spawn_locations"]["team_a"];
            case EntryKind::spawn_b: return document["spawn_locations"]["team_b"];
            case EntryKind::station: return document["stations"];
            default: throw std::runtime_error("No list for that entry kind");
            }
        }

        // Paste one copied entry into target and require: appended last in the
        // same list with the identical record, every earlier member unchanged,
        // and exactly one undo step that undo/redo cross exactly.
        Entry paste_one(LevelDocument& target, const std::string& text, EntryKind kind)
        {
            const std::string before = target.serialize();
            const bool could_undo = target.can_undo();
            const auto clip = parse(text);
            const Entry pasted = target.paste_text(text);
            require(pasted.kind == kind, "A paste did not land in the copied list");
            const auto after = parse(target.serialize());
            const auto& objects = list(after, kind);
            require(pasted.index + 1 == objects.Size() && objects[objects.Size() - 1] == clip["object"],
                "A paste did not append the copied record unchanged");
            require(target.copy_text(pasted) == text, "Copying the pasted entry did not give the same clipboard text");
            const std::string pasted_text = target.serialize();
            require(target.can_undo() && !target.can_redo() && target.undo() && target.serialize() == before &&
                target.can_undo() == could_undo && target.redo() && target.serialize() == pasted_text,
                "A paste was not exactly one undo step");
            return pasted;
        }
    }

    void check_clipboard(const fs::path& repo, const fs::path& scratch)
    {
        const auto tuning = repo / "game/content/tuning.json";
        LevelDocument source(tuning);
        source.load(repo / "game/content/levels/close_quarters.json");
        const std::string source_before = source.serialize();

        // The clipboard text: marker, list and the record exactly as written.
        const Entry wall = entry(source, EntryKind::collision, 0);
        const std::string wall_text = source.copy_text(wall);
        const auto wall_clip = parse(wall_text);
        require(wall_clip.IsObject() && wall_clip.MemberCount() == 3 && wall_clip["colourwars_level_object"] == 1 &&
            std::string(wall_clip["list"].GetString()) == "collision_objects" &&
            wall_clip["object"] == parse(source_before)["collision_objects"][0],
            "Copy did not write the marker, list and unchanged record");
        reject_without_change(source, [&] { source.copy_text(Entry{ EntryKind::level, 0, "" }); },
            "Level settings were copied");
        reject_without_change(source, [&] { source.copy_text(Entry{ EntryKind::bounds, 0, "" }); },
            "A bounds rectangle was copied");
        require(source.serialize() == source_before && !source.can_undo(), "Copying changed the source document");

        // A second stage: every copyable list pastes into it, one step each.
        LevelDocument target(tuning);
        const std::string target_before = target.serialize();
        const Entry pasted_wall = paste_one(target, wall_text, EntryKind::collision);
        require(equal(target.rectangle(pasted_wall), source.rectangle(wall)), "The pasted object moved");
        paste_one(target, source.copy_text(entry(source, EntryKind::decoration, 0)), EntryKind::decoration);
        // Close Quarters' spawns lie outside this smaller stage, so the
        // spawn comes from a third, same-sized stage.
        LevelDocument spawn_source(tuning);
        paste_one(target, spawn_source.copy_text(entry(spawn_source, EntryKind::spawn_b, 1)), EntryKind::spawn_b);
        require(target.undo() && target.undo() && target.undo() && target.serialize() == target_before &&
            !target.can_undo(), "Three pastes were not three undo steps");
        while (target.redo()) {}

        // A Movement Course station keeps its name and lesson in a stage
        // without one, and is renamed without its lesson where it is taken.
        LevelDocument course(tuning);
        course.load(repo / "game/content/levels/movement_course.json");
        Entry long_gap{};
        for (const Entry& candidate : course.entries())
        {
            if (candidate.kind == EntryKind::station && candidate.label == "Station long_gap") { long_gap = candidate; }
        }
        require(long_gap.kind == EntryKind::station, "The Movement Course lacks long_gap");
        const std::string station_text = course.copy_text(long_gap);
        require(parse(station_text)["object"].HasMember("lesson"), "The copied station lost its lesson");
        LevelDocument lesson_target(tuning);
        const Entry kept = paste_one(lesson_target, station_text, EntryKind::station);
        require(kept.label == "Station long_gap", "A station pasted where its name is free was renamed");
        const std::string course_before = course.serialize();
        const Entry renamed = course.paste_text(station_text);
        require(renamed.label == "Station long_gap_copy", "A station pasted over its own name was not renamed");
        const auto renamed_json = parse(course.serialize());
        const auto& stations = renamed_json["stations"];
        require(!stations[stations.Size() - 1].HasMember("lesson"), "A renamed station paste kept its lesson");
        require(course.undo() && course.serialize() == course_before, "A station paste was not one undo step");

        // Refusals: malformed text and other shapes, then records the
        // receiving level refuses. Each leaves document and history intact.
        const std::string target_ready = target.serialize();
        refuses([&] { target.paste_text("not json"); }, "does not hold a copied level object",
            "Malformed text was not refused as a non-object");
        for (const char* bad : { "", "not json", "{\"colourwars_level_object\": 1, \"list\": \"collision_objects\"",
            "[1, 2]", "{}", "{\"type\": \"Structure\"}",
            "{\"colourwars_level_object\": 2, \"list\": \"collision_objects\", \"object\": {}}",
            "{\"colourwars_level_object\": 1, \"list\": \"level_bounds\", \"object\": {}}",
            "{\"colourwars_level_object\": 1, \"list\": \"collision_objects\", \"object\": []}",
            "{\"colourwars_level_object\": 1, \"list\": \"collision_objects\", \"object\": {}}" })
        {
            reject_without_change(target, [&] { target.paste_text(bad); }, "Malformed clipboard text was pasted");
        }
        refuses([&] { target.paste_text("{\"colourwars_level_object\": 1, \"list\": \"level_bounds\", \"object\": {}}"); },
            "unknown level list 'level_bounds'", "An unknown list was not named");

        auto wrong_half = parse(spawn_source.copy_text(entry(spawn_source, EntryKind::spawn_a, 0)));
        wrong_half["object"]["x"].SetDouble(2800.0);
        reject_without_change(target, [&] { target.paste_text(encode(wrong_half)); },
            "A team A spawn on the right half was pasted");
        refuses([&] { target.paste_text(encode(wrong_half)); }, "Team A spawns on the left half",
            "A wrong-half spawn was not refused in the level's words");
        auto material = parse(wall_text);
        material["object"]["material"].SetString("marble");
        reject_without_change(target, [&] { target.paste_text(encode(material)); },
            "An object with an unknown material was pasted");
        auto unnamed = parse(station_text);
        unnamed["object"]["name"].SetString("Bad Name", unnamed.GetAllocator());
        reject_without_change(target, [&] { target.paste_text(encode(unnamed)); },
            "A station with an untypeable name was pasted");
        auto decoration_as_collision = parse(source.copy_text(entry(source, EntryKind::decoration, 0)));
        decoration_as_collision["list"].SetString("collision_objects");
        reject_without_change(target, [&] { target.paste_text(encode(decoration_as_collision)); },
            "A decoration was pasted into collision_objects");
        require(target.serialize() == target_ready, "A refused paste changed the receiving document");

        // The receiving stage saves and reopens with its pasted objects.
        const auto saved = scratch / "clipboard-paste.json";
        target.save(saved);
        LevelDocument reopened(tuning);
        reopened.load(saved);
        require(parse(reopened.serialize()) == parse(target.serialize()), "Pasted objects did not survive save");
        require(source.serialize() == source_before, "Pasting elsewhere changed the source document");
        std::cout << "PASS clipboard: copy/paste of a collision object, decoration, spawn and station into a second "
            "stage as one undo step each, station rename without lesson, 10 malformed-text and 4 invalid-record "
            "refusals without change, save and reopen\n";
    }
}
