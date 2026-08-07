#pragma once

#include "game/objects/player_team.h"
#include "game/objects/wep_type.h"

struct MenuPlayerSettings
{
    player_team team = player_team::NONE;
    wep_type weapon = wep_type::SPRAYER;
    int num = -1;
};
