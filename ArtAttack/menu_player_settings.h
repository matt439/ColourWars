#ifndef MENU_PLAYER_SETTINGS_H
#define MENU_PLAYER_SETTINGS_H

#include "player_team.h"
#include "wep_type.h"

struct menu_player_settings
{
    player_team team = player_team::NONE;
    wep_type weapon = wep_type::SPRAYER;
    int num = -1;
};

#endif // !MENU_PLAYER_SETTINGS_H
