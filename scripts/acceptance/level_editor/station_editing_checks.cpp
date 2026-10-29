// Station editing (ED#26): stations are listed, added, renamed, moved,
// aligned, duplicated and deleted through the same validated, undoable edits
// as every other entry, and the ED#23 shared rules (game/station_contract.h)
// refuse what the game refuses. Lessons are preserved as written.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <functional>
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

        Entry station_named(const LevelDocument& document, const std::string& name)
        {
            for (const Entry& candidate : document.entries())
            {
                if (candidate.kind == EntryKind::station && candidate.label == "Station " + name) { return candidate; }
            }
            throw std::runtime_error("Station '" + name + "' is not listed");
        }

        std::size_t station_count(const LevelDocument& document)
        {
            std::size_t count = 0;
            for (const Entry& candidate : document.entries())
            {
                if (candidate.kind == EntryKind::station) { ++count; }
            }
            return count;
        }

        const rapidjson::Value* station_json(const rapidjson::Document& document, const std::string& name)
        {
            if (!document.HasMember("stations")) { return nullptr; }
            for (const auto& station : document["stations"].GetArray())
            {
                if (std::string(station["name"].GetString()) == name) { return &station; }
            }
            return nullptr;
        }
    }

    void check_station_editing(const fs::path& repo, const fs::path& scratch)
    {
        const auto tuning = repo / "game/content/tuning.json";
        const auto course = repo / "game/content/levels/movement_course.json";
        const auto original = parse(contents(course));
        LevelDocument document(tuning);
        document.load(course);
        require(station_count(document) == 10, "The Movement Course does not list its ten stations");
        const Entry drop = station_named(document, "drop_through");
        require(equal(document.rectangle(drop), { 559.0, 715.0, 52.0, 120.0 }),
            "A station is not a character-sized marker at its top-left");

        // The inspector has name, x and y; the lesson has no rows and no edits.
        bool named = false;
        for (const Property& property : document.properties(drop))
        {
            require(!property.path.starts_with("lesson"), "A lesson field is listed in the inspector");
            named = named || (property.path == "name" && property.value == "drop_through");
        }
        require(named, "The station's name row is missing");
        reject_without_change(document, [&] { document.set_property(drop, "lesson/title", "CHANGED"); },
            "A lesson title was edited through the inspector");
        reject_without_change(document, [&] { document.mirror_copy(drop); }, "A station was mirror-copied");

        // Rename: valid, then the shared rules' refusals, each with history intact.
        document.set_property(drop, "name", "drop_lesson");
        const Entry renamed = station_named(document, "drop_lesson");
        for (const char* bad : { "Drop", "spawn_a", "spawn_b", "high_step", "", "drop lesson" })
        {
            reject_without_change(document, [&] { document.set_property(renamed, "name", bad); },
                "A refused station name was accepted");
        }
        refuses([&] { document.set_property(renamed, "name", "spawn_a"); },
            "names a station 'spawn_a', which every level already has", "The reserved refusal is not the game's");
        refuses([&] { document.set_property(renamed, "name", "high_step"); },
            "repeats the station name 'high_step'", "The repeat refusal is not the game's");

        // Move inside the lesson area; out of it is refused in the game's words.
        document.set_rectangle(renamed, { 600.0, 700.0, 52.0, 120.0 });
        require(equal(document.rectangle(renamed), { 600.0, 700.0, 52.0, 120.0 }), "A station drag did not move it");
        refuses([&] { document.set_rectangle(renamed, { 800.0, 700.0, 52.0, 120.0 }); },
            "does not contain station 'drop_lesson'", "A move out of the lesson area was not refused in the game's words");
        reject_without_change(document, [&] { document.set_rectangle(renamed, { 800.0, 700.0, 52.0, 120.0 }); },
            "A move out of the lesson area changed the document");
        document.set_rectangle(renamed, { 603.0, 712.0, 52.0, 120.0 });
        document.align_origin(renamed, 25);
        require(equal(document.rectangle(renamed), { 600.0, 700.0, 52.0, 120.0 }), "Align origin did not round a station");

        // The lesson round trip: renamed and moved, saved and reopened, the
        // lesson block is the authored one and no other station changed.
        const auto saved = scratch / "station-edited-movement-course.json";
        document.save(saved);
        LevelDocument reopened(tuning);
        reopened.load(saved);
        require(parse(reopened.serialize()) == parse(document.serialize()), "Station edits did not survive save");
        const auto reread = parse(contents(saved));
        const auto* before = station_json(original, "drop_through");
        const auto* after = station_json(reread, "drop_lesson");
        require(before && after && (*before)["lesson"] == (*after)["lesson"], "The lesson changed across edit and save");
        require((*after)["x"].GetDouble() == 600.0 && (*after)["y"].GetDouble() == 700.0, "The moved station did not save");
        for (const auto& station : original["stations"].GetArray())
        {
            const std::string name = station["name"].GetString();
            if (name == "drop_through") { continue; }
            const auto* kept = station_json(reread, name);
            require(kept && *kept == station, "Station '" + name + "' changed although it was not edited");
        }

        // Undo crosses the whole sequence back to the file as loaded.
        while (document.undo()) {}
        require(parse(document.serialize()) == original, "Undo did not restore the loaded stations");
        while (document.redo()) {}
        require(station_json(parse(document.serialize()), "drop_lesson") != nullptr, "Redo lost the renamed station");

        // Duplicate: a typeable unique name, offset, and no lesson.
        const Entry copy = document.duplicate(station_named(document, "high_step"));
        require(copy.kind == EntryKind::station && copy.label == "Station high_step_copy", "The station copy is misnamed");
        const auto* source = station_json(original, "high_step");
        require(equal(document.rectangle(copy), { (*source)["x"].GetDouble() + 50.0, (*source)["y"].GetDouble() + 50.0,
            52.0, 120.0 }), "The station copy is not offset by 50");
        const auto duplicated = parse(document.serialize());
        require(!station_json(duplicated, "high_step_copy")->HasMember("lesson"), "The station copy kept a lesson");
        const Entry second = document.duplicate(station_named(document, "high_step"));
        require(second.label == "Station high_step_copy_2", "A second station copy repeated a name");
        document.erase(second);
        document.erase(station_named(document, "high_step_copy"));
        require(station_count(document) == 10, "Deleting the copies left the wrong stations");
        require(document.undo() && station_count(document) == 11, "Undo did not restore a deleted station");
        require(document.redo() && station_count(document) == 10, "Redo did not delete the station again");

        // Add from the palette, to a stage with none: station_1, then
        // station_2; deleting both leaves no stations array.
        LevelDocument fresh(tuning);
        require(!parse(fresh.serialize()).HasMember("stations"), "The template already has stations");
        const Entry added = fresh.add_object("Station", { 1000.0, 600.0, 200.0, 100.0 });
        require(added.kind == EntryKind::station && added.label == "Station station_1" &&
            equal(fresh.rectangle(added), { 1000.0, 600.0, 52.0, 120.0 }),
            "The palette station is not station_1 at the drop point");
        const Entry next = fresh.add_object("Station", { 1200.0, 600.0, 200.0, 100.0 });
        require(next.label == "Station station_2", "A second palette station repeated a name");
        reject_without_change(fresh, [&] { fresh.add_object("Station", { -5000.0, 600.0, 200.0, 100.0 }); },
            "A station outside out_of_bounds was added");
        refuses([&] { fresh.set_rectangle(added, { 1000.0, 5000.0, 52.0, 120.0 }); },
            "outside its own out_of_bounds rectangle", "A station dragged out of bounds was not refused in the game's words");
        const auto palette_saved = scratch / "station-palette.json";
        fresh.save(palette_saved);
        LevelDocument palette_reopened(tuning);
        palette_reopened.load(palette_saved);
        require(station_count(palette_reopened) == 2, "Palette stations did not save");
        fresh.erase(next);
        fresh.erase(station_named(fresh, "station_1"));
        require(!parse(fresh.serialize()).HasMember("stations"), "Deleting the last station left an empty array");
        require(fresh.undo() && station_count(fresh) == 1, "Undo did not restore the last station");
        std::cout << "PASS station editing: list, add, rename, move, align, duplicate and delete with undo; reserved, "
            "repeated, misspelt, off-lesson and out-of-bounds refusals; Movement Course lessons survive edit and save\n";
    }
}
