// The team gate (ROAD#15): the editor authors one from the palette, lists
// its gate field in the inspector and refuses a bad gate by the game's own
// rule (game/team_gate_contract.h), on every edit and at load, in the
// game's words, before save rather than at game load.
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
            require(document.serialize() == before, "A refused gate load changed the open document");
        }
    }

    void check_team_gate(const fs::path& repo, const fs::path& scratch)
    {
        const auto tuning = repo / "game/content/tuning.json";
        const auto source = repo / "game/content/levels/turbulence_gate_trial.json";
        const std::string original = contents(source);
        LevelDocument document(tuning);
        document.load(source);
        require(document.validate().empty() && !document.dirty(), "The Gate Trial did not open clean");
        const Entry gate = named(document, "centre_gate");
        {
            const auto fields = document.properties(gate);
            const Property* field = property(fields, "gate");
            require(field != nullptr && field->value == "paint" &&
                field->label == "gate (paint: open to the team holding more of it)",
                "The inspector did not name the gate's opening rule");
        }

        // Save and reopen the shipped gate stage unchanged.
        const auto copy = scratch / "turbulence_gate_trial.json";
        document.save(copy);
        LevelDocument reopened(tuning);
        reopened.load(copy);
        require(parse(reopened.serialize()) == parse(original) && !reopened.dirty() &&
            contents(source) == original, "The Gate Trial lost its gate across save and reopen");

        // Every edit the game would refuse, through the inspector.
        for (const auto& [path, invalid] : {
            std::pair{ "gate", "wind" }, { "gate", "Paint" },
            { "collision_type", "STRUCTURE_JUMP_THROUGH" } })
        {
            reject_without_change(document, [&] { document.set_property(gate, path, invalid); },
                "The editor accepted a gate the game refuses");
        }
        document.set_property(gate, "paintable_faces/left", "false");
        document.set_property(gate, "paintable_faces/right", "false");
        reject_without_change(document, [&] { document.set_property(gate, "paintable_faces/top", "false"); },
            "The editor accepted a gate no paint could open");
        // Moving a gate onto a spawn body is refused like the game's startup check.
        reject_without_change(document, [&] { document.set_rectangle(gate, { 40.0, 1000.0, 80.0, 240.0 }); },
            "The editor accepted a gate over a spawn");
        // Clearing the gate leaves an ordinary paintable block.
        document.set_property(gate, "gate", "");
        {
            const auto cleared = parse(document.serialize());
            const auto& object = cleared["collision_objects"][static_cast<rapidjson::SizeType>(gate.index)];
            require(!object.HasMember("gate") && document.validate().empty(),
                "Clearing the gate did not leave a plain paintable block");
        }
        while (document.undo()) {}
        require(parse(document.serialize()) == parse(original), "Gate edits did not undo to the shipped stage");

        // The palette entry is a gate the game would load.
        LevelDocument fresh(tuning);
        const Entry added = fresh.add_object("TeamGate", { 1000, 600, 80, 240 });
        const auto fresh_json = parse(fresh.serialize());
        const auto& object = fresh_json["collision_objects"][static_cast<rapidjson::SizeType>(added.index)];
        require(std::string(object["gate"].GetString()) == "paint" &&
            std::string(object["collision_type"].GetString()) == "STRUCTURE_PAINTABLE" &&
            std::string(object["material"].GetString()) == "eraser" &&
            !object["paintable_faces"]["bottom"].GetBool() && object["paintable_faces"]["top"].GetBool() &&
            fresh.validate().empty(), "The palette team gate is not a solid, paintable eraser gate");

        // Every invalid gate shape, as a file: refused at load with the
        // shared rule's words.
        const auto variant = [&](const char* name, const std::function<void(rapidjson::Value&, rapidjson::Document&)>& edit)
        {
            auto document_json = parse(original);
            for (auto& entry : document_json["collision_objects"].GetArray())
            {
                if (std::string(entry["name"].GetString()) == "centre_gate") { edit(entry, document_json); }
            }
            const auto path = scratch / (std::string("gate-") + name + ".json");
            write(path, encode(document_json));
            return path;
        };
        LevelDocument checker(tuning);
        refuses_file(checker, variant("unknown", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["gate"].SetString("wind"); }), "gate: gate must be 'paint'", "An unknown gate opened");
        refuses_file(checker, variant("number", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["gate"].SetInt(1); }), "gate: gate must be 'paint'", "A numeric gate opened");
        refuses_file(checker, variant("one-way", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["collision_type"].SetString("STRUCTURE_JUMP_THROUGH"); entry.RemoveMember("material"); }),
            "a gate is solid: use STRUCTURE_PAINTABLE collision", "A one-way gate opened");
        refuses_file(checker, variant("plain", [](rapidjson::Value& entry, rapidjson::Document&)
            {
                entry["type"].SetString("Structure"); entry["collision_type"].SetString("STRUCTURE");
                entry.RemoveMember("paintable_faces"); entry["material"].SetString("ground");
            }), "gate belongs only on a StructurePaintable", "A gate on a plain structure opened");
        refuses_file(checker, variant("unpainted", [](rapidjson::Value& entry, rapidjson::Document&)
            {
                for (const char* face : { "left", "top", "right", "bottom" }) { entry["paintable_faces"][face].SetBool(false); }
            }), "a gate needs a paintable face", "A gate with no paintable face opened");
        refuses_file(checker, variant("moving", [](rapidjson::Value& entry, rapidjson::Document& owner)
            {
                rapidjson::Value motion(rapidjson::kObjectType);
                auto& allocator = owner.GetAllocator();
                motion.AddMember("type", "horizontal", allocator);
                motion.AddMember("travel_x", 40.0, allocator);
                motion.AddMember("period", 4.0, allocator);
                motion.AddMember("phase", 0.0, allocator);
                entry.AddMember("motion", motion, allocator);
            }), "a gate does not move", "A moving gate opened");
        refuses_file(checker, variant("spawn", [](rapidjson::Value& entry, rapidjson::Document&)
            { entry["rectangle"]["x"].SetDouble(40.0); entry["rectangle"]["y"].SetDouble(1000.0); }),
            "gate: a gate must not overlap a spawn body", "A gate over a spawn opened");
        std::cout << "PASS team gate inspector field, palette entry, shared gate refusals and Gate Trial save/reopen\n";
    }
}
