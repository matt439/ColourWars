#include "game/pch.h"
#include "game/objects/weapon_sprayer.h"

using namespace DirectX;
using namespace MattMath;
using namespace weapon_consts;

WeaponSprayer::WeaponSprayer(player_team team,
    int player_num,
    const Colour& team_colour,
    wep_type type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    RelativeVelocityWeapon(DETAILS_SPRAYER,
        DETAILS_RELATIVE_SPRAYER,
        team, player_num, team_colour, type, player_center,
        render_resources, audio_resources, dt,
        color, rotation, origin, effects, layer_depth)
{

}