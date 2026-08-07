#pragma once

enum class wep_type
{
    SPRAYER,
    SNIPER,
    ROLLER,  //no aiming, always touches ground. alternate fire when jumping (splash short forward)
    MISTER,
    BUCKET,
    RANDOM_PRIMARY,
    GRENADE_STD,
    MINE,
    CURLING,
    WATER,
    NONE,
    MAX_PRIM_WEP = RANDOM_PRIMARY
};
