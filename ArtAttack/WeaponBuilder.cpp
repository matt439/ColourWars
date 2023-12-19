#include "pch.h"
#include "WeaponBuilder.h"

using namespace MattMath;
using namespace DirectX;

std::unique_ptr<Weapon> WeaponBuilder::build_weapon(
    player_team team,
    int player_num,
    const Colour& team_colour,
    wep_type type,
    const Vector2F& player_center,
    SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    const float* dt)
{
    switch (type)
    {
    case wep_type::SPRAYER:
        return std::make_unique<WeaponSprayer>(
            team, player_num, team_colour, type, player_center,
            sprite_batch, resource_manager, dt);
    case wep_type::SNIPER:
        return std::make_unique<WeaponSniper>(team, player_num, team_colour,
            type, player_center,
            sprite_batch, resource_manager, dt);
    case wep_type::ROLLER:
        return std::make_unique<WeaponRoller>(team, player_num, team_colour,
            type, player_center,
            sprite_batch, resource_manager, dt);
    case wep_type::MISTER:
        return std::make_unique<WeaponMister>(team, player_num, team_colour,
            type, player_center,
            sprite_batch, resource_manager, dt);
    case wep_type::BUCKET:
        return std::make_unique<WeaponBucket>(team, player_num, team_colour,
            type, player_center,
            sprite_batch, resource_manager, dt);
    default:
        throw std::exception("Invalid weapon type");
    }
}