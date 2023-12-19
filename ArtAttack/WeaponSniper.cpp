#include "pch.h"
#include "WeaponSniper.h"

using namespace DirectX;
using namespace MattMath;
using namespace weapon_consts;


WeaponSniper::WeaponSniper(player_team team,
    int player_num,
    const Colour& team_colour,
    wep_type type,
    const Vector2F& player_center,
    SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    Weapon(DETAILS_SNIPER,
        team, player_num, team_colour, type, player_center,
        sprite_batch, resource_manager, dt,
        color, rotation, origin, effects, layer_depth)
{

}
void WeaponSniper::handle_shoot_sound(bool shooting_this_update, bool holding_shoot)
{
    if (shooting_this_update)
    {
        this->_sound_bank->play_wave(this->_details.shoot_sound_name,
            this->_details.shoot_sound_volume);
    }
}