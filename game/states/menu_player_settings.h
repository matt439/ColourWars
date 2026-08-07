#ifndef MENU_PLAYER_SETTINGS_H
#define MENU_PLAYER_SETTINGS_H

#include "game/objects/player_team.h"
#include "game/objects/wep_type.h"

struct MenuPlayerSettings
{
    player_team team = player_team::NONE;
    wep_type weapon = wep_type::SPRAYER;
    int num = -1;
};

#endif // !MENU_PLAYER_SETTINGS_H
