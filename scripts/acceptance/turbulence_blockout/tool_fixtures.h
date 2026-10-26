#pragma once

#include "scripts/acceptance/turbulence_blockout/fixtures.h"
#include "game/objects/collision_object_type.h"
#include <iomanip>
#include <limits>

namespace turbulence_blockout_acceptance
{
    inline constexpr std::array<WeaponType, 5> concrete_tools{
        WeaponType::sprayer, WeaponType::sniper, WeaponType::roller, WeaponType::mister, WeaponType::bucket };
    inline const char* tool_name(WeaponType tool)
    {
        switch (tool)
        {
        case WeaponType::sprayer: return "sprayer";
        case WeaponType::sniper: return "sniper";
        case WeaponType::roller: return "roller";
        case WeaponType::mister: return "mister";
        case WeaponType::bucket: return "bucket";
        default: throw std::runtime_error("Tool matrix needs a concrete primary");
        }
    }
    inline PlayerInputData firing(float angle, float movement = 0.0f, bool shoot = true)
    {
        PlayerInputData actions = input(movement);
        actions.primary_shoot = shoot;
        actions.shoot_direction_requested = true;
        actions.shoot_angle = angle;
        actions.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return actions;
    }
    inline bool flying(const Run& run)
    {
        for (const auto* object : run.scene.collision_objects())
        { if (is_projectile(to_collision_type(object->tag()))) { return true; } }
        return false;
    }
}
