// The paint ferry's drive fields: the editor authors them in the inspector
// and refuses a bad one by the game's own rule (horizontal_motion_contract::
// decode_drive and validate_drive), before save rather than at game load.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace level_editor_acceptance
{
    namespace
    {
        Entry named(const LevelDocument& document, const std::string& name)
        {
            for (const Entry& candidate : document.entries())
            {
                if (candidate.kind == EntryKind::collision && candidate.label == name) { return candidate; }
            }
            throw std::runtime_error("Expected entry '" + name + "' is absent");
        }

        const Property* property(const std::vector<Property>& properties, const std::string& path)
        {
            for (const Property& candidate : properties)
            {
                if (candidate.path == path) { return &candidate; }
            }
            return nullptr;
        }

        const rapidjson::Value& motion_of(const LevelDocument& document, const Entry& entry)
        {
            static rapidjson::Document held;
            held = parse(document.serialize());
            return held["collision_objects"][static_cast<rapidjson::SizeType>(entry.index)]["motion"];
        }

        // A file the game would refuse at load is refused by the editor at
        // load too, naming the shared rule's own words.
        void refuses_file(LevelDocument& document, const fs::path& path, const std::string& expected,
            const char* message)
        {
            const std::string before = document.serialize();
            bool rejected = false;
            try { document.load(path); }
            catch (const std::exception& error)
            {
                rejected = true;
                require(std::string(error.what()).find(expected) != std::string::npos,
                    std::string(message) + ": diagnostic was '" + error.what() + "'");
            }
            require(rejected, message);
            require(document.serialize() == before, "A refused drive load changed the open document");
        }
    }

    void check_paint_drive(const fs::path& repo, const fs::path& scratch)
    {
        const auto tuning = repo / "game/content/tuning.json";
        const auto source = repo / "game/content/levels/turbulence_ferry_trial.json";
        const std::string original = contents(source);
        LevelDocument document(tuning);
        document.load(source);
        require(document.validate().empty() && !document.dirty(), "The Ferry Trial did not open clean");
        const Entry ferry = named(document, "ferry_a");
        const Entry other = named(document, "ferry_b");
        {
            const auto fields = document.properties(ferry);
            const Property* drive = property(fields, "motion/drive");
            const Property* heads = property(fields, "motion/team_a_heads");
            const Property* phase = property(fields, "motion/phase");
            require(drive != nullptr && drive->value == "paint" && drive->label == "motion/drive (cycle or paint)" &&
                heads != nullptr && heads->value == "right" && heads->label == "motion/team_a_heads (right or left)" &&
                phase != nullptr && phase->label == "motion/phase (paint: 0 left end to 0.5 right end)",
                "The inspector did not name the ferry's drive, heading and paint phase range");
        }

        // Save and reopen the shipped ferry stage unchanged.
        const auto copy = scratch / "turbulence_ferry_trial.json";
        document.save(copy);
        LevelDocument reopened(tuning);
        reopened.load(copy);
        require(parse(reopened.serialize()) == parse(original) && !reopened.dirty() &&
            contents(source) == original, "The Ferry Trial lost drive fields across save and reopen");

        // Valid inspector edits: heading flips, cycle sheds its heading, paint
        // gains team A heading right, and both undo exactly.
        document.set_property(ferry, "motion/team_a_heads", "left");
        require(std::string(motion_of(document, ferry)["team_a_heads"].GetString()) == "left",
            "The inspector did not flip team A's heading");
        document.set_property(ferry, "motion/drive", "cycle");
        require(std::string(motion_of(document, ferry)["drive"].GetString()) == "cycle" &&
            !motion_of(document, ferry).HasMember("team_a_heads"), "Switching to cycle kept a paint-only heading");
        document.set_property(ferry, "motion/phase", "0.75");
        reject_without_change(document, [&] { document.set_property(ferry, "motion/drive", "paint"); },
            "Switching to paint accepted a return-leg phase");
        document.set_property(ferry, "motion/phase", "0.5");
        document.set_property(ferry, "motion/drive", "paint");
        require(std::string(motion_of(document, ferry)["team_a_heads"].GetString()) == "right",
            "Switching to paint did not carry team A heading right");
        document.set_property(ferry, "motion/drive", "");
        require(!motion_of(document, ferry).HasMember("drive") && !motion_of(document, ferry).HasMember("team_a_heads"),
            "Clearing the drive did not return the mover to the cycle default");
        {
            const auto fields = document.properties(ferry);
            const Property* drive = property(fields, "motion/drive");
            require(drive != nullptr && drive->value == "cycle" && property(fields, "motion/team_a_heads") == nullptr,
                "An absent drive was not shown as the cycle default");
        }
        const std::string cleared = document.serialize();
        document.set_property(ferry, "motion/drive", "cycle");
        require(document.serialize() == cleared, "Typing the cycle default over an absent drive added a field");
        while (document.undo()) {}
        require(parse(document.serialize()) == parse(original), "Drive edits did not undo to the shipped stage");

        // Every invalid drive shape, as an inspector edit.
        for (const auto& [path, invalid] : {
            std::pair{ "motion/drive", "wind" }, { "motion/drive", "Paint" },
            { "motion/team_a_heads", "up" }, { "motion/team_a_heads", "" }, { "motion/phase", "0.75" } })
        {
            reject_without_change(document, [&] { document.set_property(other, path, invalid); },
                "The editor accepted a drive the game refuses");
        }
        document.set_property(other, "motion/drive", "cycle");
        reject_without_change(document, [&] { document.set_property(other, "motion/team_a_heads", "right"); },
            "The editor accepted a heading on a cycle");
        document.undo();

        // A new mover from the palette can be made a ferry in the inspector.
        LevelDocument fresh(tuning);
        const Entry mover = fresh.add_object("MovingPaintable", { 600, 600, 320, 20 });
        fresh.set_property(mover, "motion/drive", "paint");
        fresh.set_property(mover, "motion/team_a_heads", "left");
        require(std::string(motion_of(fresh, mover)["drive"].GetString()) == "paint" &&
            std::string(motion_of(fresh, mover)["team_a_heads"].GetString()) == "left" && fresh.validate().empty(),
            "A palette mover could not be authored as a paint ferry");

        // Every invalid drive shape, as a file: refused at load with the
        // shared rule's words, before the author could save it.
        const auto variant = [&](const char* name, const std::function<void(rapidjson::Value&, rapidjson::Document&)>& edit)
        {
            auto document_json = parse(original);
            for (auto& object : document_json["collision_objects"].GetArray())
            {
                if (std::string(object["name"].GetString()) == "ferry_a") { edit(object["motion"], document_json); }
            }
            const auto path = scratch / (std::string("drive-") + name + ".json");
            write(path, encode(document_json));
            return path;
        };
        LevelDocument checker(tuning);
        refuses_file(checker, variant("no-heading", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion.RemoveMember("team_a_heads"); }), "a paint drive needs team_a_heads",
            "A paint drive without a heading opened");
        refuses_file(checker, variant("bad-heading", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion["team_a_heads"].SetString("up"); }), "team_a_heads must be 'right' or 'left'",
            "A paint drive heading up opened");
        refuses_file(checker, variant("unknown-drive", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion["drive"].SetString("wind"); motion.RemoveMember("team_a_heads"); }),
            "drive must be 'cycle' or 'paint'", "An unknown drive opened");
        refuses_file(checker, variant("cycle-heading", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion["drive"].SetString("cycle"); }), "team_a_heads belongs only on a paint drive",
            "A cycle with a heading opened");
        refuses_file(checker, variant("absent-drive-heading", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion.RemoveMember("drive"); }), "team_a_heads belongs only on a paint drive",
            "A heading on the default cycle opened");
        refuses_file(checker, variant("return-leg", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion["phase"].SetDouble(0.75); }), "starts at phase 0 (left end) to 0.5 (right end)",
            "A paint drive on its return leg opened");
        refuses_file(checker, variant("number-drive", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion["drive"].SetInt(1); }), "drive must be text", "A numeric drive opened");
        refuses_file(checker, variant("number-heading", [](rapidjson::Value& motion, rapidjson::Document&)
            { motion["team_a_heads"].SetInt(1); }), "team_a_heads must be text", "A numeric heading opened");
        std::cout << "PASS paint ferry drive inspector edits, shared drive refusals and Ferry Trial save/reopen\n";
    }
}
