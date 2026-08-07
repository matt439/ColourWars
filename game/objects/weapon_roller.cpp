#include "game/pch.h"
#include "game/objects/weapon_roller.h"

using namespace DirectX;
using namespace mattmath;
using namespace weapon_consts;


WeaponRoller::WeaponRoller(PlayerTeam team,
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
    Weapon(DETAILS_ROLLER,
        team, player_num, team_colour, type, player_center,
        render_resources, audio_resources, dt,
        color, rotation, origin, effects, layer_depth)
{

}

void WeaponRoller::update_movement_and_rotation(PlayerInputData /*input*/,
    const Vector2F& player_center,
    const Vector2F& /*player_velocity*/,
    bool player_facing_right)
{
    if (player_facing_right)  
    {
        this->set_rotation(PI / 4.0f);
    }
    else //player facing left
    {
        this->set_rotation(3.0f * PI / 4.0f);
    }
    this->set_gun_player_aligned(true);
    

    this->set_player_center(player_center);

    //gun facing left
    if (facing_left(this->get_rotation()))
    {
        this->set_invert_y(true);
    }
    //gun facing right
    else
    {
        this->set_invert_y(false);
    }
}

mattmath::Colour WeaponRoller::get_draw_colour() const
{
    // The roller tints to the team colour while it is laying paint.
    return this->shooting_this_update_
        ? this->get_team_colour()
        : colour_consts::WHITE;
}
