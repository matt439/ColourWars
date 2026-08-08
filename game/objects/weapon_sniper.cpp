#include "game/pch.h"
#include "game/objects/weapon_sniper.h"

using namespace DirectX;
using namespace mattmath;
using namespace weapon_consts;
using namespace artattack;


WeaponSniper::WeaponSniper(PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    Weapon(DETAILS_SNIPER,
        team, player_num, team_colour, type, player_center,
        render_resources, audio_resources,        color, rotation, origin, effects, layer_depth)
{

}
void WeaponSniper::handle_shoot_sound(bool shooting_this_update, bool /*holding_shoot*/)
{
    if (shooting_this_update)
    {
        this->sound_bank_->play_wave(this->shoot_sound_,
            this->details_.shoot_sound_volume);
    }
}