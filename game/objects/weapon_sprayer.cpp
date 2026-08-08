#include "game/pch.h"
#include "game/objects/weapon_sprayer.h"

using namespace DirectX;
using namespace mattmath;
using namespace weapon_consts;
using namespace artattack;

WeaponSprayer::WeaponSprayer(PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    RelativeVelocityWeapon(DETAILS_SPRAYER,
        DETAILS_RELATIVE_SPRAYER,
        team, player_num, team_colour, type, player_center,
        render_resources, audio_resources,        color, rotation, origin, flip, layer_depth)
{

}