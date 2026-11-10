#pragma once

#include "game/objects/player_team.h"
#include "game/objects/sub_type.h"
#include "game/objects/weapon_type.h"
#include "game/match/player_control.h"

struct SeatSetup
{
    PlayerTeam team = PlayerTeam::none;
    WeaponType weapon = WeaponType::sprayer;
    // The sub carried beside it (ROAD#11). The grenade is the default;
    // Weapon Select's loadout row chooses it, and the remembered setup in
    // the save, the Debug plan and replays carry the choice.
    SubType sub = SubType::grenade;
    // Stable human input slot; -1 for an unclaimed human or a bot. The vector
    // position in MatchSetup identifies the roster participant.
    int num = -1;
    PlayerController controller = PlayerController::human;
    BotDifficulty difficulty = BotDifficulty::normal;
    // handicaps::valid; 0 is none. A seat keeps it through bot/human changes.
    int handicap = 0;
};
