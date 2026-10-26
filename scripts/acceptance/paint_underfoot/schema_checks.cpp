#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include "game/objects/level_object_builder.h"
#include "game/tunnelling_limits.h"
#include <iostream>

namespace paint_underfoot_acceptance
{
    namespace
    {
        template<class Edit> void rejects(World& world, const char* key, Edit edit)
        {
            bool contextual = false;
            try { (void)world.variant(edit); }
            catch (const std::runtime_error& error)
            {
                const std::string text = error.what();
                contextual = text.find("paint-underfoot-variant.json") != std::string::npos &&
                    text.find("paint_underfoot") != std::string::npos && text.find(key) != std::string::npos;
            }
            require(contextual, "Invalid underfoot data did not reject with source, owner and field context");
        }
        std::string stage_rejection(World& world)
        {
            const auto stage = labrador::parse_json(R"([{"type":"Structure","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":0,"y":0,"width":1,"height":10000},"collision_type":"STRUCTURE","colour":"WHITE"}])", "paint-underfoot-stage.json");
            LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
            try { check_stage_tunnelling(world.tuning, builder, stage.root(), dt); }
            catch (const std::runtime_error& error) { return error.what(); }
            return {};
        }
    }
    void check_schema_and_bounds(World& world)
    {
        const auto missing = world.variant([](rapidjson::Document& value) { value["player"].RemoveMember("paint_underfoot"); });
        const auto& defaults = missing.player().paint_underfoot;
        require(!defaults.enabled && defaults.own_speed_multiplier == 1.0f && defaults.enemy_speed_multiplier == 1.0f &&
            defaults.own_ammo_regen_multiplier == 1.0f && defaults.enemy_ammo_regen_multiplier == 1.0f,
            "Missing old block did not produce disabled neutral compatibility defaults");
        rejects(world, "paint_underfoot", [](rapidjson::Document& value) { value["player"]["paint_underfoot"].SetNull(); });
        rejects(world, "enabled", [](rapidjson::Document& value) { value["player"]["paint_underfoot"]["enabled"].SetString("false", value.GetAllocator()); });
        for (const char* key : { "own_speed_multiplier", "own_ammo_regen_multiplier", "enemy_speed_multiplier", "enemy_ammo_regen_multiplier" })
        {
            rejects(world, key, [&](rapidjson::Document& value) { value["player"]["paint_underfoot"].RemoveMember(key); });
            rejects(world, key, [&](rapidjson::Document& value) { value["player"]["paint_underfoot"][key].SetBool(true); });
            rejects(world, key, [&](rapidjson::Document& value) { value["player"]["paint_underfoot"][key].SetDouble(1e100); });
            rejects(world, key, [&](rapidjson::Document& value) { value["player"]["paint_underfoot"][key].SetFloat(-0.1f); });
        }
        for (const char* key : { "own_speed_multiplier", "own_ammo_regen_multiplier" })
        { rejects(world, key, [&](rapidjson::Document& value) { value["player"]["paint_underfoot"][key].SetFloat(0.99f); }); }
        rejects(world, "enemy_speed_multiplier", [](rapidjson::Document& value) { value["player"]["paint_underfoot"]["enemy_speed_multiplier"].SetFloat(0); });
        for (const char* key : { "enemy_speed_multiplier", "enemy_ammo_regen_multiplier" })
        { rejects(world, key, [&](rapidjson::Document& value) { value["player"]["paint_underfoot"][key].SetFloat(1.01f); }); }
        rejects(world, "own_speed_multiplier", [](rapidjson::Document& value) { value["player"]["paint_underfoot"]["own_speed_multiplier"].SetFloat(3e38f); });
        rejects(world, "own_ammo_regen_multiplier", [](rapidjson::Document& value)
        { value["player"]["paint_underfoot"]["own_ammo_regen_multiplier"].SetFloat(3e38f); value["weapons"]["ammo_regen_rate"].SetFloat(2); });
        rejects(world, "enemy_speed_multiplier", [](rapidjson::Document& value)
        { value["player"]["x_initial_velocity"].SetFloat(1e-30f); value["player"]["paint_underfoot"]["enemy_speed_multiplier"].SetFloat(1e-30f); });
        world.configure(false, 4.0f);
        require(stage_rejection(world).empty(), "Disabled speed trial changed the original stage displacement bound");
        world.configure(true, 4.0f);
        const std::string unsafe = stage_rejection(world);
        require(unsafe.find("paint-underfoot-stage.json") != std::string::npos &&
            unsafe.find("horizontally") != std::string::npos &&
            unsafe.find("player.paint_underfoot.own_speed_multiplier") != std::string::npos,
            "Enabled/retained boosted speed bypassed the actual per-stage bound or lost its tuning diagnostic");
        world.configure(false);
        std::cout << "PASS underfoot schema: optional-old/default-off, typed/ranged/derived-overflow inputs and actual stage bounds\n";
    }
}
