#pragma once

enum class WeaponType
{
    sprayer,
    sniper,
    roller,  // Fixed downward aim; fires a short-lived contact volume.
    mister,
    bucket,
    random_primary,
    grenade_std,
    mine,
    curling,
    water,
    none,
    max_prim_wep = random_primary
};
