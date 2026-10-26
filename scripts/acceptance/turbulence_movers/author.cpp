#include "game/pch.h"
#include "editor/level_document.h"
#include <rapidjson/document.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace turbulence_movers_acceptance
{
    namespace
    {
        void author_require(bool condition, const std::string& message)
        {
            if (!condition) { throw std::runtime_error("Turbulence mover author: " + message); }
        }

        std::string author_read(const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            author_require(input.good(), "cannot read " + path.string());
            const std::string bytes{ std::istreambuf_iterator<char>(input), {} };
            author_require(!input.bad(), "incomplete read of " + path.string());
            return bytes;
        }

        rapidjson::Document author_parse(const std::string& bytes)
        {
            rapidjson::Document document;
            document.Parse(bytes.data(), bytes.size());
            author_require(!document.HasParseError() && document.IsObject(), "source or reopened output is not a JSON object");
            return document;
        }

        struct AuthorSource
        {
            std::filesystem::path path;
            std::string bytes;
        };

        void verify_mover(const rapidjson::Value& value, const char* name, double anchor_x, double phase)
        {
            author_require(value.IsObject() && value.HasMember("name") && value["name"].IsString() &&
                std::string_view(value["name"].GetString(), value["name"].GetStringLength()) == name &&
                value.HasMember("type") && value["type"].IsString() && std::string_view(value["type"].GetString()) == "StructurePaintable" &&
                value.HasMember("collision_type") && value["collision_type"].IsString() &&
                std::string_view(value["collision_type"].GetString()) == "STRUCTURE_JUMP_THROUGH",
                "saved palette entry lost its name or one-way paintable type");
            author_require(value.HasMember("rectangle") && value["rectangle"].IsObject() &&
                value.HasMember("motion") && value["motion"].IsObject(), "saved mover lost its rectangle or motion");
            const auto& box = value["rectangle"];
            for (const auto& [key, expected] : std::array<std::pair<const char*, double>, 4>{
                std::pair{ "x", anchor_x }, std::pair{ "y", 720.0 },
                std::pair{ "width", 320.0 }, std::pair{ "height", 20.0 } })
            {
                author_require(box.HasMember(key) && box[key].IsNumber() && box[key].GetDouble() == expected,
                    "saved mover anchor differs at " + std::string(key));
            }
            const auto& motion = value["motion"];
            author_require(motion.HasMember("type") && motion["type"].IsString() &&
                std::string_view(motion["type"].GetString()) == "horizontal", "saved mover is not horizontal");
            for (const auto& [key, expected] : std::array<std::pair<const char*, double>, 3>{
                std::pair{ "travel_x", 320.0 }, std::pair{ "period", 4.0 }, std::pair{ "phase", phase } })
            {
                author_require(motion.HasMember(key) && motion[key].IsNumber() && motion[key].GetDouble() == expected,
                    "saved mover motion differs at " + std::string(key));
            }
            author_require(value.HasMember("material") && value["material"].IsString() &&
                std::string_view(value["material"].GetString()) == "ruler" &&
                value.HasMember("paintable_faces") && value["paintable_faces"].IsObject(),
                "saved mover lost the public palette material or credit faces");
            const auto& faces = value["paintable_faces"];
            for (const char* face : { "left", "top", "right", "bottom" })
            {
                author_require(faces.HasMember(face) && faces[face].IsBool() && faces[face].GetBool(),
                    "saved mover changed the public palette's credited " + std::string(face) + " face");
            }
        }
    }

    void author_trial(const std::filesystem::path& static_stage, const std::filesystem::path& tuning,
        const std::filesystem::path& output)
    {
        using colourwars_editor::Entry;
        using colourwars_editor::EntryKind;
        using colourwars_editor::LevelDocument;
        author_require(!std::filesystem::exists(output), "refusing existing output " + output.string());
        author_require(static_stage.filename() == "turbulence_blockout.json", "expected the preserved Turbulence static candidate");
        constexpr std::array<const char*, 7> names{ "close_quarters.json", "king_of_the_hill.json", "turbulence.json",
            "movement_course.json", "crossfeed.json", "press_tower.json", "turbulence_blockout.json" };
        std::vector<AuthorSource> original;
        for (const char* name : names)
        {
            const auto path = static_stage.parent_path() / name;
            original.push_back({ path, author_read(path) });
        }
        const std::string tuning_bytes = author_read(tuning);
        const auto source = author_parse(original.back().bytes);
        author_require(source.HasMember("collision_objects") && source["collision_objects"].IsArray() &&
            source["collision_objects"].Size() == 20 && source.HasMember("display_name") &&
            source["display_name"].IsString() && source.HasMember("include_in_random") &&
            source["include_in_random"].IsBool() && !source["include_in_random"].GetBool(),
            "source must retain its twenty-object explicit-only static blockout");

        LevelDocument document(tuning);
        document.load(static_stage);
        const Entry metadata{ EntryKind::level, 0, "" };
        document.set_property(metadata, "display_name", "Turbulence Yard - Mover Trial");
        document.set_property(metadata, "include_in_random", "false");
        const std::array<double, 2> anchors{ 600.0, 2760.0 };
        const std::array<const char*, 2> mover_names{ "mover_a", "mover_b" };
        const std::array<const char*, 2> phases{ "0", "0.5" };
        for (std::size_t index = 0; index < anchors.size(); ++index)
        {
            const Entry mover = document.add_object("MovingJumpThrough", { anchors[index], 720, 320, 20 });
            document.set_property(mover, "name", mover_names[index]);
            document.set_property(mover, "motion/travel_x", "320");
            document.set_property(mover, "motion/period", "4");
            document.set_property(mover, "motion/phase", phases[index]);
            const auto guide = document.motion_guide(mover);
            author_require(guide && guide->anchor.x == anchors[index] && guide->anchor.y == 720 &&
                guide->anchor.width == 320 && guide->anchor.height == 20 && guide->travel_envelope.width == 640 &&
                guide->initial_pose.x == anchors[index] + (index == 0 ? 0.0 : 320.0),
                "actual editor motion guide differs from the paired trial");
        }
        author_require(document.validate().empty() && document.dirty(), "actual editor rejected the trial or lost its edits");
        if (!output.parent_path().empty()) { std::filesystem::create_directories(output.parent_path()); }
        author_require(!std::filesystem::exists(output), "output appeared before actual editor save");
        document.save(output);
        author_require(!document.dirty() && document.path() == output, "actual editor save did not adopt its fresh output path");
        LevelDocument reopened(tuning);
        reopened.load(output);
        author_require(reopened.validate().empty() && !reopened.dirty() && reopened.serialize() == document.serialize(),
            "actual editor save/reopen changed the trial or its admitted motion corridors");
        const auto saved = author_parse(author_read(output));
        author_require(saved.MemberCount() == source.MemberCount() && saved.HasMember("collision_objects") &&
            saved["collision_objects"].IsArray() && saved["collision_objects"].Size() == 22,
            "editor save did not retain exactly twenty original objects plus two movers");
        for (auto member = source.MemberBegin(); member != source.MemberEnd(); ++member)
        {
            const std::string_view key(member->name.GetString(), member->name.GetStringLength());
            if (key == "display_name" || key == "include_in_random" || key == "collision_objects") { continue; }
            author_require(saved.HasMember(member->name) && saved[member->name] == member->value,
                "original static metadata/geometry changed at " + std::string(key));
        }
        for (rapidjson::SizeType index = 0; index < source["collision_objects"].Size(); ++index)
        {
            author_require(saved["collision_objects"][index] == source["collision_objects"][index],
                "original collision object or its credit faces changed at " + std::to_string(index));
        }
        author_require(saved.HasMember("display_name") && saved["display_name"].IsString() &&
            std::string_view(saved["display_name"].GetString()) == "Turbulence Yard - Mover Trial" &&
            saved.HasMember("include_in_random") && saved["include_in_random"].IsBool() && !saved["include_in_random"].GetBool(),
            "saved trial lost its explicit-only display metadata");
        verify_mover(saved["collision_objects"][20], "mover_a", 600.0, 0.0);
        verify_mover(saved["collision_objects"][21], "mover_b", 2760.0, 0.5);
        for (const auto& snapshot : original)
        {
            author_require(author_read(snapshot.path) == snapshot.bytes, "original arena bytes changed at " + snapshot.path.string());
        }
        author_require(author_read(tuning) == tuning_bytes, "source tuning bytes changed");
        std::cout << "PASS actual LevelDocument mover trial save/reopen: twenty static objects preserved, two public-palette "
            "one-way paintables, paired320-unit/4-second motion, seven original arena files and tuning byte-identical; "
            "trial remains explicit-only and unregistered pending qualification\n";
    }
}
