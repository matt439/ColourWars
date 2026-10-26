#include "game/pch.h"
#include "scripts/acceptance/turbulence_slopes/fixtures.h"
#include <array>
#include <iostream>

namespace turbulence_slopes_acceptance
{
    SourceGuard::SourceGuard(const fs::path& content, const fs::path& tuning, const fs::path& source)
    {
        for (const char* name : { "close_quarters.json", "king_of_the_hill.json", "turbulence.json", "movement_course.json",
            "crossfeed.json", "press_tower.json", "turbulence_blockout.json", "turbulence_mover_trial.json" })
        {
            const auto path = content / "levels" / name;
            sources.emplace_back(path, read_text(path));
        }
        sources.emplace_back(tuning, read_text(tuning));
        sources.emplace_back(source, read_text(source));
    }

    void SourceGuard::check() const
    {
        for (const auto& [path, bytes] : sources)
        { require(read_text(path) == bytes, "Shoulder trial changed an original arena/tuning/source: " + path.string()); }
    }

    void author_trial(const fs::path& content, const fs::path& source, const fs::path& tuning, const fs::path& output)
    {
        using namespace colourwars_editor;
        SourceGuard originals(content, tuning, source);
        require(!fs::exists(output), "Refusing an existing isolated shoulder trial");
        LevelDocument document(tuning); document.load(source);
        const Entry metadata{ EntryKind::level, 0, "" };
        document.set_property(metadata, "display_name", "Turbulence Yard - Shoulder Trial");
        document.set_property(metadata, "include_in_random", "false");
        for (const bool side_b : { false, true })
        {
            const double x = side_b ? 2600.0 : 1240.0;
            const std::string suffix = side_b ? "_b" : "_a";
            const Entry ramp = document.add_object(side_b ? "StructureRampLeft" : "StructureRampRight", { x, 800, 80, 40 });
            document.set_rectangle(ramp, { x, 800, 160, 80 });
            document.set_property(ramp, "name", "shoulder_ramp" + suffix);
            document.set_property(ramp, "material", "set_square");
            const Entry shoe = document.add_object("StructurePaintable", { x, 880, 80, 12 });
            document.set_rectangle(shoe, { x, 880, 160, 24 });
            document.set_property(shoe, "name", "shoulder_shoe" + suffix);
            document.set_property(shoe, "material", "eraser");
            for (const char* face : { "left", "top", "right", "bottom" })
            { document.set_property(shoe, std::string("paintable_faces/") + face, "true"); }
        }
        require(document.validate().empty(), "Actual editor rejected the four-object shoulder composite");
        fs::create_directories(output.parent_path()); document.save(output);
        LevelDocument reopened(tuning); reopened.load(output);
        require(!document.dirty() && reopened.validate().empty() && reopened.serialize() == document.serialize(),
            "Shoulder composite changed during actual editor save/reopen");
        rapidjson::Document before, after;
        const auto before_bytes = read_text(source), after_bytes = read_text(output);
        before.Parse(before_bytes.c_str()); after.Parse(after_bytes.c_str());
        require(!before.HasParseError() && !after.HasParseError() && before["collision_objects"].Size() == 22 &&
            after["collision_objects"].Size() == 26, "Shoulder source must be twenty static objects plus two qualified movers");
        for (rapidjson::SizeType index = 0; index < 22; ++index)
        { require(before["collision_objects"][index] == after["collision_objects"][index], "Shoulder author changed an existing collision object"); }
        for (auto member = before.MemberBegin(); member != before.MemberEnd(); ++member)
        {
            const std::string key = member->name.GetString();
            if (key == "display_name" || key == "include_in_random" || key == "collision_objects") { continue; }
            require(after.HasMember(member->name) && after[member->name] == member->value,
                "Shoulder author changed original metadata/spawns/bounds: " + key);
        }
        originals.check();
        std::cout << "PASS actual LevelDocument shoulder author: four palette/setter objects, save/reopen, twenty-two original objects "
            "and all eight source arenas/tuning unchanged; isolated composite uses existing ramp/flat paint contracts\n";
    }
}
