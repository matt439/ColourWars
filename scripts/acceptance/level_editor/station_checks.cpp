// Stations (ED#23): the editor refuses a station the game refuses, by the
// game's own rules (game/station_contract.h), on every edit and at load, in
// the game's words, before save rather than at game load.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace level_editor_acceptance
{
    namespace
    {
        Entry out_of_bounds(const LevelDocument& document)
        {
            for (const Entry& candidate : document.entries())
            {
                if (candidate.kind == EntryKind::bounds && candidate.label == "Out of bounds") { return candidate; }
            }
            throw std::runtime_error("Out of bounds entry is absent");
        }

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
    }

    void check_stations(const fs::path& repo, const fs::path& scratch)
    {
        const auto tuning = repo / "game/content/tuning.json";

        // Shrinking Press Tower's out_of_bounds past `crown` (1774, 620) is
        // refused like the game's startup check, with history intact.
        const auto press_tower = repo / "game/content/levels/press_tower.json";
        const std::string original = contents(press_tower);
        LevelDocument document(tuning);
        document.load(press_tower);
        require(document.validate().empty() && !document.dirty(), "Press Tower did not open clean");
        const Entry bounds = out_of_bounds(document);
        document.set_rectangle(bounds, { -100.0, -90.0, 3800.0, 1890.0 });
        require(document.can_undo(), "A valid bounds edit left no undo step");
        reject_without_change(document, [&] { document.set_rectangle(bounds, { -100.0, 650.0, 3800.0, 1150.0 }); },
            "The editor accepted bounds that leave a station outside");
        refuses([&] { document.set_rectangle(bounds, { -100.0, 650.0, 3800.0, 1150.0 }); },
            "stations[2] ('crown') puts a station 'crown' at (1774.000000, 620.000000), outside its own "
            "out_of_bounds rectangle", "The bounds refusal is not in the game's words");
        require(document.undo() && !document.can_undo() && parse(document.serialize()) == parse(original),
            "The refused bounds edit lost the earlier undo step");

        // Every invalid station shape, as a file: refused at load with the
        // shared rules' words, leaving the open document as it was.
        const auto course = repo / "game/content/levels/movement_course.json";
        const std::string course_text = contents(course);
        const auto variant = [&](const char* name, const std::function<void(rapidjson::Value&, rapidjson::Document&)>& edit)
        {
            auto document_json = parse(course_text);
            for (auto& entry : document_json["stations"].GetArray())
            {
                if (std::string(entry["name"].GetString()) == "drop_through") { edit(entry, document_json); }
            }
            const auto path = scratch / (std::string("station-") + name + ".json");
            write(path, encode(document_json));
            return path;
        };
        LevelDocument checker(tuning);
        checker.load(course);
        require(checker.validate().empty(), "The Movement Course did not open clean");
        const std::string before = checker.serialize();
        const auto refuses_file = [&](const fs::path& path, const std::string& expected, const char* message)
        {
            refuses([&] { checker.load(path); }, expected, message);
            require(checker.serialize() == before, "A refused station load changed the open document");
        };
        refuses_file(variant("spelling", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["name"].SetString("Drop"); }),
            "names a station 'Drop'; station names are lower-case letters, digits and '_'", "A capital station opened");
        refuses_file(variant("reserved", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["name"].SetString("spawn_b"); }),
            "names a station 'spawn_b', which every level already has", "A reserved station opened");
        refuses_file(variant("repeat", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["name"].SetString("high_step"); }), "repeats the station name 'high_step'",
            "A repeated station opened");
        refuses_file(variant("outside", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["y"].SetDouble(-5000.0); }), "outside its own out_of_bounds rectangle",
            "A station outside the bounds opened");
        refuses_file(variant("flat-lesson", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["lesson"]["area"]["height"].SetDouble(0.0); }),
            ".lesson.area needs finite coordinates and positive width and height", "A flat lesson opened");
        refuses_file(variant("lesson-off-camera", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["lesson"]["area"]["x"].SetDouble(-10.0); }),
            "leaves the camera bounds; no player could stand in all of it.", "A lesson off camera opened");
        refuses_file(variant("lesson-elsewhere", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["lesson"]["area"]["x"].SetDouble(1000.0); }),
            "does not contain station 'drop_through'; going to the station must show its lesson.",
            "A lesson away from its station opened");
        refuses_file(variant("blank-title", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["lesson"]["title"].SetString(" \n"); }),
            ".lesson.title is empty; a lesson needs a title and a line to read.", "A blank lesson opened");

        // Every shipped stage with stations still opens and saves clean.
        for (const char* stage : { "crossfeed", "close_quarters", "press_tower", "movement_course" })
        {
            LevelDocument shipped(tuning);
            shipped.load(repo / "game/content/levels" / (std::string(stage) + ".json"));
            require(shipped.validate().empty(), std::string(stage) + " has a station the editor refuses");
            const auto copy = scratch / (std::string("station-saved-") + stage + ".json");
            shipped.save(copy);
            LevelDocument reopened(tuning);
            reopened.load(copy);
            require(parse(reopened.serialize()) == parse(shipped.serialize()),
                std::string(stage) + " lost its stations across save and reopen");
        }
        std::cout << "PASS stations: bounds edit past a station refused with history intact, eight station "
            "refusals in the game's words, shipped stations save and reopen\n";
    }
}
