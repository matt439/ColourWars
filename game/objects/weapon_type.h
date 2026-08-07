#pragma once

enum class WeaponType
{
    sprayer,
    sniper,
    roller,  //no aiming, always touches ground. alternate fire when jumping (splash short forward)
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
