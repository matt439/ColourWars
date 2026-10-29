// Mirror copy (ED#24): Edit > Mirror copy reflects one object across the
// out_of_bounds centre as one validated undo step. Each shipped `_a` object is
// mirrored with its authored `_b` partner removed, and the copy must be that
// partner exactly; paint ferries, level settings and bounds are refused.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace level_editor_acceptance
{
    namespace
    {
        struct Found { EntryKind kind; std::size_t index; };

        std::string name_of(const rapidjson::Value& object)
        {
            return object.HasMember("name") && object["name"].IsString() ? object["name"].GetString() : "";
        }

        // The JSON of one entry, from the document's own serialization.
        rapidjson::Document record(const LevelDocument& document, const Entry& entry)
        {
            const auto json = parse(document.serialize());
            const rapidjson::Value* values = nullptr;
            if (entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b)
                values = &json["spawn_locations"][entry.kind == EntryKind::spawn_a ? "team_a" : "team_b"];
            else
                values = &json[entry.kind == EntryKind::collision ? "collision_objects" : "non_collision_objects"];
            rapidjson::Document result;
            result.CopyFrom((*values)[static_cast<rapidjson::SizeType>(entry.index)], result.GetAllocator());
            return result;
        }

        Entry named(const LevelDocument& document, EntryKind kind, const std::string& name)
        {
            for (const Entry& candidate : document.entries())
                if (candidate.kind == kind && candidate.label == name) return candidate;
            throw std::runtime_error("Expected object '" + name + "' is absent");
        }

        // Mirror one entry and require exactly one undo step that undo and
        // redo move across unchanged.
        Entry mirror_once(LevelDocument& document, const Entry& source, const std::string& what)
        {
            const std::string before = document.serialize();
            const Entry copy = document.mirror_copy(source);
            const std::string after = document.serialize();
            require(after != before && document.can_undo() && !document.can_redo(),
                what + ": mirror copy left no undo step");
            require(document.validate().empty(), what + ": mirror copy left the document invalid");
            require(document.undo() && document.serialize() == before, what + ": one undo did not remove the copy");
            require(document.redo() && document.serialize() == after, what + ": redo did not restore the copy");
            return copy;
        }
    }

    void check_mirror_copy(const fs::path& repo, const fs::path& /*scratch*/)
    {
        const auto tuning = repo / "game/content/tuning.json";
        const auto levels = repo / "game/content/levels";

        // Every shipped `_a` object with an authored `_b` partner in the same
        // list. Crossfeed's clouds are deliberately different sizes; the Ferry
        // Trial's ferries are paint-driven and refused below.
        const std::set<std::pair<std::string, std::string>> authored_differences{ { "crossfeed", "cloud_a" } };
        std::set<std::string> kinds;
        int mirrored = 0;
        for (const auto& file : fs::directory_iterator(levels))
        {
            if (file.path().extension() != ".json") continue;
            const std::string stage = file.path().stem().string();
            const auto source = parse(contents(file.path()));
            for (const auto& [kind, list] : { std::pair{ EntryKind::collision, "collision_objects" },
                std::pair{ EntryKind::decoration, "non_collision_objects" } })
            {
                if (!source.HasMember(list)) continue;
                for (const auto& object : source[list].GetArray())
                {
                    const std::string name = name_of(object);
                    if (!name.ends_with("_a") || authored_differences.contains({ stage, name })) continue;
                    const std::string partner = name.substr(0, name.size() - 1) + "b";
                    bool paired = false;
                    for (const auto& other : source[list].GetArray()) paired |= name_of(other) == partner;
                    if (!paired) continue;
                    const std::string what = stage + " " + name;
                    LevelDocument document(tuning);
                    document.load(file.path());
                    const auto expected = record(document, named(document, kind, partner));
                    document.erase(named(document, kind, partner));
                    const Entry original = named(document, kind, name);
                    const auto& motion = object.HasMember("motion") ? object["motion"] : object;
                    if (object.HasMember("motion") && motion.HasMember("drive") &&
                        std::string(motion["drive"].GetString()) == "paint")
                    {
                        reject_without_change(document, [&] { document.mirror_copy(original); },
                            "Mirror copy accepted a paint ferry");
                        try { document.mirror_copy(original); }
                        catch (const std::exception& error)
                        {
                            require(std::string(error.what()).find("paint ferries") != std::string::npos,
                                what + ": the ferry refusal does not say why: " + error.what());
                        }
                        kinds.insert("refused ferry");
                        continue;
                    }
                    const Entry copy = mirror_once(document, original, what);
                    require(copy.kind == kind && copy.label == partner, what + ": the copy is not named " + partner);
                    require(record(document, copy) == expected, what + ": the mirror copy is not its authored partner");
                    const std::string type = object["type"].GetString();
                    kinds.insert(object.HasMember("motion") ? "mover" : type == "StructureRamp" ?
                        type + " " + object["collision_type"].GetString() : type);
                    ++mirrored;
                }
            }
        }
        for (const char* kind : { "StructurePaintable", "Visual", "StructureRamp STRUCTURE_RAMP_RIGHT",
            "StructureRamp STRUCTURE_RAMP_LEFT", "FixedSlabPaintable", "mover", "refused ferry" })
            require(kinds.contains(kind), std::string("No shipped mirror pair covered ") + kind);

        // The worked example: the Mover Trial's left + right is 4000, so
        // mover_a (x 600, travel 320, width 320, phase 0) becomes x 2760, phase 0.5.
        {
            LevelDocument document(tuning);
            document.load(levels / "turbulence_mover_trial.json");
            document.erase(named(document, EntryKind::collision, "mover_b"));
            const auto copy = record(document, mirror_once(document,
                named(document, EntryKind::collision, "mover_a"), "Mover Trial mover_a"));
            require(copy["rectangle"]["x"].GetDouble() == 2760.0 && copy["motion"]["phase"].GetDouble() == 0.5,
                "mover_a did not mirror to x 2760, phase 0.5");
        }

        // Spawns: every Team A point mirrors onto its Team B partner and back.
        // Each team keeps its four points, so the copy is appended.
        for (const char* stage : { "press_tower", "crossfeed", "turbulence_blockout" })
        {
            LevelDocument document(tuning);
            document.load(levels / (std::string(stage) + ".json"));
            for (std::size_t index = 0; index < 4; ++index)
            {
                for (const auto& [from, to] : { std::pair{ EntryKind::spawn_a, EntryKind::spawn_b },
                    std::pair{ EntryKind::spawn_b, EntryKind::spawn_a } })
                {
                    const auto expected = record(document, entry(document, to, index));
                    const Entry copy = mirror_once(document, entry(document, from, index),
                        std::string(stage) + " spawn " + std::to_string(index));
                    require(copy.kind == to && record(document, copy) == expected,
                        std::string(stage) + ": a spawn did not mirror onto the other team's partner");
                    document.erase(copy);
                }
            }
        }

        // What shipped pairs do not show: one-sided faces swap, an unsuffixed
        // name gains " mirror", and a drifting visual shifts half a cycle.
        {
            LevelDocument document(tuning);
            document.load(levels / "press_tower.json");
            const Entry stair = named(document, EntryKind::collision, "stair_1_a");
            document.set_property(stair, "paintable_faces/left", "false");
            document.set_property(stair, "name", "lonely_stair");
            const auto copy = record(document, mirror_once(document,
                named(document, EntryKind::collision, "lonely_stair"), "one-sided stair"));
            require(std::string(copy["name"].GetString()) == "lonely_stair mirror",
                "An unsuffixed name did not gain ' mirror'");
            require(copy["paintable_faces"]["left"].GetBool() && !copy["paintable_faces"]["right"].GetBool(),
                "A one-sided paint face did not swap sides");
            require(copy["rectangle"]["x"].GetDouble() == 3600.0 - 1000.0 - 160.0,
                "The one-sided stair did not reflect across Press Tower's centre");
        }
        {
            LevelDocument document(tuning);
            document.load(levels / "close_quarters.json");
            const Rect world = document.rectangle(Entry{ EntryKind::bounds, 0, "" });
            const Entry cloud = named(document, EntryKind::decoration, "cloud_4");
            const auto source = record(document, cloud);
            const auto copy = record(document, mirror_once(document, cloud, "Close Quarters cloud_4"));
            require(std::abs(copy["drift"]["phase"].GetDouble() - (source["drift"]["phase"].GetDouble() - 0.5)) < 1e-12,
                "A drifting visual's phase did not wrap half a cycle");
            require(copy["rectangle"]["x"].GetDouble() == world.x + (world.x + world.width) -
                source["rectangle"]["x"].GetDouble() - source["rectangle"]["width"].GetDouble(),
                "A drifting visual did not reflect its anchor");
            // A phase just under 0.5 rounds to a whole cycle, which wraps to 0.
            document.set_property(cloud, "drift/phase", "0.4999999999999");
            const auto wrapped = record(document, mirror_once(document, cloud, "Close Quarters cloud_4 near 0.5"));
            require(wrapped["drift"]["phase"].GetDouble() == 0.0,
                "A drift phase just under 0.5 did not wrap to 0");
        }

        // Level settings and bounds have no mirror; a copy that would leave
        // the arena or cross a spawn rule is refused by ordinary validation.
        {
            LevelDocument document(tuning);
            document.load(levels / "press_tower.json");
            reject_without_change(document, [&] { document.mirror_copy(Entry{ EntryKind::level, 0, "" }); },
                "Mirror copy accepted the level settings");
            reject_without_change(document, [&] { document.mirror_copy(Entry{ EntryKind::bounds, 0, "" }); },
                "Mirror copy accepted a bounds rectangle");
            const Entry spawn = entry(document, EntryKind::spawn_a, 0);
            document.set_rectangle(spawn, { 1790.0, 1000.0, 52.0, 120.0 });
            reject_without_change(document, [&] { document.mirror_copy(spawn); },
                "Mirror copy put a Team B spawn left of the centre");
        }

        std::cout << "PASS mirror copy: " << mirrored << " shipped _a objects reproduce their _b partners "
            "(rectangles, ramps both hands, a fixed slab, a cycle mover, visuals) and every spawn its partner, "
            "each in one undo step; one-sided faces, unsuffixed names and drift reflect; a paint ferry, level "
            "settings, bounds and a centre-crossing spawn are refused\n";
    }
}
