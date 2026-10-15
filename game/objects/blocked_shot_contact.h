#pragma once

#include "engine/math/vector2f.h"
#include "engine/render/colour.h"

// A stopped shot's resolved world contact. The normal points away from cover;
// this snapshot remains detached from the projectile and its moving owner.
struct BlockedShotContact
{
    mattmath::Vector2F position;
    mattmath::Vector2F normal;
    labrador::Colour team_colour;
    int player_num = -1;
};
