#include "pch.h"
#include "WeaponRoller.h"

using namespace DirectX;
using namespace MattMath;
using namespace weapon_consts;


WeaponRoller::WeaponRoller(player_team team,
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
    Weapon(DETAILS_ROLLER,
        team, player_num, team_colour, type, player_center,
        sprite_batch, resource_manager, dt,
        color, rotation, origin, effects, layer_depth)
{

}

void WeaponRoller::update_movement_and_rotation(player_input input,
    const Vector2F& player_center,
    const Vector2F& player_velocity,
    bool player_facing_right)
{
    const float dt = this->get_dt();

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
    if (this->facing_left(this->get_rotation()))
    {
        this->set_invert_y(true);
    }
    //gun facing right
    else
    {
        this->set_invert_y(false);
    }
}

//std::vector<std::unique_ptr<ICollisionGameObject>>
//Weapon::update_and_get_projectiles(player_input input,
//    const Vector2F& player_center,
//    const Vector2F& player_velocity,
//    bool player_facing_right)
//{
//    this->update_movement_and_rotation(input, player_center,
//        player_velocity, player_facing_right);
//
//    if (this->check_if_shooting_and_ammo_update(input, player_center,
//        player_velocity, player_facing_right))
//    {
//        return this->shoot(input.shoot_direction);
//    }
//    else
//    {
//        return std::vector<std::unique_ptr<ICollisionGameObject>>();
//    }
//}