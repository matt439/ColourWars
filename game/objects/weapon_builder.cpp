#include "game/pch.h"
#include "game/objects/weapon_builder.h"

using namespace mattmath;
using namespace DirectX;
using namespace artattack;

std::unique_ptr<Weapon> WeaponBuilder::build_weapon(
    const GameTuning* tuning,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources)
{
    switch (type)
    {
    case WeaponType::sprayer:
    case WeaponType::mister:
        return std::make_unique<RelativeVelocityWeapon>(
            tuning, team, player_num, team_colour, type, player_center,
            render_resources, audio_resources);
    case WeaponType::sniper:
        return std::make_unique<WeaponSniper>(tuning, team, player_num,
            team_colour, type, player_center,
            render_resources, audio_resources);
    case WeaponType::roller:
        return std::make_unique<WeaponRoller>(tuning, team, player_num,
            team_colour, type, player_center,
            render_resources, audio_resources);
    case WeaponType::bucket:
        return std::make_unique<WeaponBucket>(tuning, team, player_num,
            team_colour, type, player_center,
            render_resources, audio_resources);
    default:
        throw std::runtime_error("Weapon type " +
            std::to_string(static_cast<int>(type)) +
            " has no builder: ./tuning.json describes the five a player can "
            "pick, and this is not one of them.");
    }
}
