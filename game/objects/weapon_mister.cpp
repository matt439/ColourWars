#include "game/pch.h"
#include "game/objects/weapon_mister.h"

using namespace DirectX;
using namespace mattmath;
using namespace weapon_consts;

WeaponMister::WeaponMister(PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    RelativeVelocityWeapon(DETAILS_MISTER,
        DETAILS_RELATIVE_MISTER,
        team, player_num, team_colour, type, player_center,
        render_resources, audio_resources, dt,
        color, rotation, origin, effects, layer_depth)
{

}