#ifndef WEP_TYPE_H
#define WEP_TYPE_H

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

#endif // !WEP_TYPE_H
