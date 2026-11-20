#pragma once

#include "scripts/acceptance/support/json_fixture.h"

namespace acceptance
{
    inline rapidjson::Value& fill_fixture_object(rapidjson::Document& document, std::string_view name)
    {
        for (auto& object : document["collision_objects"].GetArray())
            if (name == object["name"].GetString()) { return object; }
        throw std::runtime_error("Fill fixture has no object named " + std::string(name));
    }

    // Both loaders see identical variants of the shipped trial. Some defects
    // are structural; named references and exposed geometry need the level.
    template<class Check>
    void check_invalid_paint_fill(const std::filesystem::path& levels, const std::filesystem::path& scratch,
        Check check)
    {
        const rapidjson::Document trial = read_document(levels / "turbulence_fill_trial.json");
        const auto variant = [&](const char* name, bool schema_accepts, auto edit)
        {
            rapidjson::Document level;
            level.CopyFrom(trial, level.GetAllocator());
            edit(level);
            const auto path = scratch / (std::string("fill-") + name + ".json");
            write_document(level, path);
            check(path, level, schema_accepts);
        };
        variant("missing-target", false, [](auto& d) { d["paint_fill"].RemoveMember("target"); });
        variant("numeric-object", false, [](auto& d) { d["paint_fill"]["target"]["object"].SetInt(0); });
        variant("empty-trigger", false, [](auto& d) { d["paint_fill"]["trigger"]["faces"].Clear(); });
        variant("repeated-face", false, [](auto& d) { d["paint_fill"]["trigger"]["faces"][1].SetString("left"); });
        variant("unknown-face", false, [](auto& d) { d["paint_fill"]["target"]["face"].SetString("front"); });
        variant("empty-name", false, [](auto& d) { d["paint_fill"]["trigger"]["object"].SetString(""); });
        variant("unknown-object", true, [](auto& d) { d["paint_fill"]["target"]["object"].SetString("missing"); });
        variant("ambiguous-name", true, [](auto& d)
            { fill_fixture_object(d, "crossover_a")["name"].SetString("centre_baffle"); });
        variant("nonpaintable", true, [](auto& d) { d["paint_fill"]["target"]["object"].SetString("boundary_wall_left"); });
        variant("disabled-face", true, [](auto& d) { fill_fixture_object(d, "centre_baffle")["paintable_faces"]["top"].SetBool(false); });
        variant("overlap", true, [](auto& d) { d["paint_fill"]["target"]["face"].SetString("right"); });
        variant("buried-target", true, [](auto& d)
        {
            const auto& baffle = fill_fixture_object(d, "centre_baffle")["rectangle"];
            auto& cover = fill_fixture_object(d, "crossover_b")["rectangle"];
            cover["x"].SetDouble(baffle["x"].GetDouble());
            cover["y"].SetDouble(baffle["y"].GetDouble() - cover["height"].GetDouble());
        });
        // The right side is still paintable. A fully buried left side makes
        // the two-face trigger invalid even though its aggregate area is > 0.
        variant("buried-trigger-face", true, [](auto& d)
        {
            const auto& baffle = fill_fixture_object(d, "centre_baffle")["rectangle"];
            auto& shoulder = fill_fixture_object(d, "crossover_a")["rectangle"];
            shoulder["y"].SetDouble(baffle["y"].GetDouble());
            shoulder["height"].SetDouble(baffle["height"].GetDouble());
        });
        for (const char* endpoint : { "trigger", "target" })
        {
            rapidjson::Document moving = read_document(levels / "turbulence_mover_trial.json");
            rapidjson::Value fill(trial["paint_fill"], moving.GetAllocator());
            fill[endpoint]["object"].SetString("mover_a");
            moving.AddMember("paint_fill", fill, moving.GetAllocator());
            const auto path = scratch / (std::string("fill-moving-") + endpoint + ".json");
            write_document(moving, path);
            check(path, moving, true);
        }
    }
}
