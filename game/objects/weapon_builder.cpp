#include "game/pch.h"
#include "game/objects/weapon_builder.h"

using namespace mattmath;
using namespace DirectX;

std::unique_ptr<Weapon> WeaponBuilder::build_weapon(
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const float* dt)
{
    switch (type)
    {
    case WeaponType::sprayer:
        return std::make_unique<WeaponSprayer>(
            team, player_num, team_colour, type, player_center,
            render_resources, audio_resources, dt);
    case WeaponType::sniper:
        return std::make_unique<WeaponSniper>(team, player_num, team_colour,
            type, player_center,
            render_resources, audio_resources, dt);
    case WeaponType::roller:
        return std::make_unique<WeaponRoller>(team, player_num, team_colour,
            type, player_center,
            render_resources, audio_resources, dt);
    case WeaponType::mister:
        return std::make_unique<WeaponMister>(team, player_num, team_colour,
            type, player_center,
            render_resources, audio_resources, dt);
    case WeaponType::bucket:
        return std::make_unique<WeaponBucket>(team, player_num, team_colour,
            type, player_center,
            render_resources, audio_resources, dt);
    default:
        throw std::exception("Invalid weapon type");
    }
}