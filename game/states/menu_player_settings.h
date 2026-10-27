#pragma once

#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"
#include "game/player_control.h"

struct MenuPlayerSettings
{
    PlayerTeam team = PlayerTeam::none;
    WeaponType weapon = WeaponType::sprayer;
    int num = -1;
    PlayerController controller = PlayerController::human;
    BotDifficulty difficulty = BotDifficulty::normal;
    // handicaps::valid; 0 is none. A seat keeps it through bot/human changes.
    int handicap = 0;
};
