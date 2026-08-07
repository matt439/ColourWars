#pragma once

#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"

struct MenuPlayerSettings
{
    PlayerTeam team = PlayerTeam::none;
    WeaponType weapon = WeaponType::sprayer;
    int num = -1;
};
