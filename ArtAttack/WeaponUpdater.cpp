#include "pch.h"
#include "WeaponUpdater.h"

////using namespace DirectX::SimpleMath;
//using namespace MattMath;
//
//WeaponUpdater::WeaponUpdater(const float* dt) : Updater(dt)
//{
//    //this->_projectile_builder = std::make_unique<ProjectileBuilder>();
//    //this->_projectile_updater = std::make_unique<ProjectileUpdater>(dt);
//}
//
//void WeaponUpdater::update(Weapon* weapon, const weapon_update& params)
//{
//    ////if input requesting weapon type change
//    //if (params.input.primary != this->p_prim_type && params.input.primary != wep_type::NONE)
//    //{
//    //    this->p_prim_info = this->get_wep_info(params.input.primary);
//    //    this->p_prim_type = params.input.primary;
//    //}
//
//    if (weapon->get_details().movement == wep_movement::ON_GROUND)
//    {
//        if (params.player_facing_right)
//        {
//            weapon->set_rotation(weapon_consts::PI / 4.0f);
//        }
//        else //player facing left
//        {
//            weapon->set_rotation(3.0f * weapon_consts::PI / 4.0f);
//        }
//        weapon->set_gun_player_aligned(true);
//    }
//    else //if (this->p_prim_info.movement == wep_movement::FREE_ROTATE)
//    {
//
//        if ((params.player_facing_right &&
//            params.input.shoot_direction.x > 0.0f) ||
//            (!params.player_facing_right &&
//                params.input.shoot_direction.x < 0.0f))
//        {
//            weapon->set_gun_player_aligned(true);
//        }
//        else
//        {
//            weapon->set_gun_player_aligned(false);
//        }
//
//        if (weapon->get_gun_player_aligned())
//        {
//            weapon->set_rotation(atan2f(params.input.shoot_direction.y,
//                params.input.shoot_direction.x));
//
//        }
//        else
//        {
//            if (params.player_facing_right)
//            {
//                weapon->set_rotation(0.0f);
//            }
//            else
//            {
//                weapon->set_rotation(weapon_consts::PI);
//            }
//        }
//    }
//
//    weapon->set_position(params.player_center);
//
//    //gun facing left
//    if (weapon->facing_left(weapon->get_rotation()))
//    {
//        weapon->set_invert_y(true);
//    }
//    //gun facing right
//    else
//    {
//        weapon->set_invert_y(false);
//    }
//
//    bool normal_gun_ok_to_shoot = params.input.primary_shoot &&
//        weapon->get_gun_player_aligned();
//    bool roller_ok_to_shoot = weapon->get_movement() ==
//        wep_movement::ON_GROUND &&
//        params.input.primary_shoot; // NOTE: changed this from raw_shooting to primary shoot
//    bool ok_to_shoot = normal_gun_ok_to_shoot || roller_ok_to_shoot;
//
//    if (weapon->get_shoot_timer() > weapon->get_shoot_interval() &&
//        weapon->get_ammo() > 0.0f &&
//        ok_to_shoot)
//    {
//        shoot(weapon, params);
//
//        weapon->set_shoot_timer(0.0f);
//        weapon->alter_ammo(-weapon->get_ammo_usage());
//        if (weapon->get_ammo() < 0.0f)
//        {
//            weapon->set_ammo(0.0f);
//        }
//        weapon->set_ammo_timer(0.0f);
//    }
//
//    //ammo regen
//    if (weapon->get_ammo_timer() >= weapon_consts::AMMO_REGEN_DELAY)
//    {
//        weapon->alter_ammo(weapon_consts::AMMO_REGEN_RATE * this->get_dt());
//        if (weapon->get_ammo() > 1.0f)
//        {
//            weapon->set_ammo(1.0f);
//        }
//    }
//    weapon->alter_ammo_timer(this->get_dt());
//    weapon->alter_shoot_timer(this->get_dt());
//
//    this->delete_collided_projs(weapon);
//    this->update_projectiles(weapon);
//}
//
//void WeaponUpdater::update_projectiles(Weapon* weapon)
//{
//    for (int i = 0; i < weapon->get_num_projectiles(); i++)
//    {
//        this->_projectile_updater->update(weapon->get_projectile(i));
//    }
//}
//
//void WeaponUpdater::shoot(Weapon* weapon, const weapon_update& params)
//{
//    Vector2F launch_velocity;
//    if (! weapon->is_relative_velocity_weapon())
//    {
//        launch_velocity = calculate_projectile_launch_velocity(
//            params.input.shoot_direction, weapon->get_starting_vel_length());
//    }
//    //player velocity is added
//    else
//    {
//        add_player_velocity add_player_vel =
//            dynamic_cast<RelativeVelocityWeapon*>(weapon)->
//            get_add_player_velocity();
//
//        launch_velocity = calculate_projectile_launch_velocity(
//            params.input.shoot_direction, weapon->get_starting_vel_length(),
//            params.player_velocity, add_player_vel,
//            weapon->get_player_vel_amount());
//    }
//    
//    projectile_info proj_info;
//    proj_info.position = weapon->get_nozzle_pos();
//    proj_info.velocity = launch_velocity;
//    proj_info.team = weapon->get_team();
//    proj_info.player_num = weapon->get_player_num();
//    proj_info.team_colour = weapon->get_team_colour();
//    proj_info.type = weapon->get_projectile_type(weapon->get_type());
//
//    weapon->add_projectile(this->_projectile_builder->
//        build_projectile(proj_info));
//}
//
//Vector2F WeaponUpdater::calculate_projectile_launch_velocity(
//    const Vector2F& shoot_direction,
//    float starting_velocity)
//{
//    Vector2F result = shoot_direction * starting_velocity;
//    return result;
//}
//
//Vector2F WeaponUpdater::calculate_projectile_launch_velocity(
//    const Vector2F& shoot_direction,
//    float starting_velocity,
//    const Vector2F& player_velocity,
//    add_player_velocity add_player_vel,
//    float player_vel_amount)
//{
//    Vector2F result = shoot_direction * starting_velocity;
//    
//    if (add_player_vel == add_player_velocity::X_AND_Y)
//    {
//        result += player_velocity * player_vel_amount;
//    }
//    else if (add_player_vel == add_player_velocity::X_ONLY)
//    {
//        result.x += player_velocity.x * player_vel_amount;
//    }
//    else //if (add_player_vel == add_player_velocity::Y_ONLY)
//    {
//        result.y += player_velocity.y * player_vel_amount;
//    }
//    return result;
//}
//
//void WeaponUpdater::delete_collided_projs(Weapon* weapon)
//{
//    std::vector<Projectile>* projs = weapon->get_projectiles();
//    int i = 0;
//    while (i < weapon->get_num_projectiles())
//    {
//        Projectile& proj = (*projs)[i];
//        if (proj.get_collided() ||
//            proj.get_for_deletion())
//        {
//            (*projs)[i] = (*projs)[projs->size() - 1];
//            projs->pop_back();
//        }
//        else
//        {
//            i++;
//        }
//    }
//}