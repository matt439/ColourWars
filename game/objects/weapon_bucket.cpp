#include "game/pch.h"
#include "game/objects/weapon_bucket.h"

using namespace DirectX;
using namespace MattMath;
using namespace weapon_consts;

WeaponBucket::WeaponBucket(player_team team,
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
    RelativeVelocityWeapon(DETAILS_BUCKET,
        DETAILS_RELATIVE_BUCKET,
        team, player_num, team_colour, type, player_center,
        render_resources, audio_resources, dt,
        color, rotation, origin, effects, layer_depth)
{

}
void WeaponBucket::handle_shoot_sound(bool shooting_this_update, bool /*holding_shoot*/)
{
    if (shooting_this_update)
    {
        this->sound_bank_->play_wave(this->shoot_sound_,
            this->details_.shoot_sound_volume);
    }
}