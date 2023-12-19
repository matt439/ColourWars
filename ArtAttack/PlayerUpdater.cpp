#include "pch.h"
#include "PlayerUpdater.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

//PlayerUpdater::PlayerUpdater(const float* dt) : Updater(dt)
//{
//    this->_weapon_updater =
//        std::make_unique<WeaponUpdater>(dt);
//    //this->_camera = std::make_unique<Camera>();
//}
//
//void PlayerUpdater::update(Player* player)
//{
//    if (player->get_health() <= 0.0f &&
//        player->get_state() == player_state::ALIVE)
//    {
//        player->set_state(player_state::DEAD);
//    }
//    if (player->get_state() == player_state::ALIVE)
//    {
//        this->update_movement(player);
//
//        weapon_update wep_args;
//        wep_args.input = player->get_input();
//        wep_args.player_center = player->get_center();
//        wep_args.player_facing_right = player->get_facing_right();
//        wep_args.player_velocity = player->get_velocity();
//        this->_weapon_updater->update(player->get_primary(), wep_args);
//    }
//    else if (player->get_state() == player_state::DEAD)
//    {
//        if (player->get_respawn_timer() > player_consts::RESPAWN_DELAY)
//        {
//            player->set_state(player_state::READY_TO_RESPAWN);
//            player->set_respawn_timer(0.0f);
//        }
//        player->alter_respawn_timer(this->get_dt());
//    }
//    if (player->get_input().toggle_debug)
//    {
//        if (player->get_showing_debug())
//		{
//			player->set_showing_debug(false);
//		}
//		else
//		{
//			player->set_showing_debug(true);
//		}
//    }
//	//this->update_camera(player);
//}
//
//void PlayerUpdater::update_movement(Player* player)
//{
//    player_input input = player->get_input();
//    float x_input = input.x_movement;
//    float dt = this->get_dt();
//    //If the player's velocity is zero and the user is requesting to move the player
//    if (player->get_velocity_x() == 0 && x_input != 0)
//    {
//        player->set_velocity_x(player_consts::X_INITIAL_VELOCITY * x_input);
//    }
//    //If accelerating in the same direction as currently moving. right
//    else if (player->get_velocity_x() > 0 && x_input > 0)
//    {
//        player->alter_velocity_x(player_consts::X_ACCELERATION * dt * x_input);
//        if (player->get_velocity_x() > player_consts::MAX_VELOCITY.x)
//        {
//            player->set_velocity_x(player_consts::MAX_VELOCITY.x);
//        }
//    }
//    //If accelerating in the same direction as currently moving. left
//    else if (player->get_velocity_x() < 0 && x_input < 0)
//    {
//        player->alter_velocity_x(player_consts::X_ACCELERATION * dt * x_input);
//        if (player->get_velocity_x() < -player_consts::MAX_VELOCITY.x)
//        {
//            player->set_velocity_x(-player_consts::MAX_VELOCITY.x);
//        }
//    }
//    //If accelerating in the opposite direction as currently moving. right
//    else if (player->get_velocity_x() > 0 && x_input < 0)
//    {
//        player->alter_velocity_x(player_consts::X_DEACCELERATION * dt * x_input);
//        player->alter_velocity_x(player_consts::X_ACCELERATION * dt * x_input);
//    }
//    //If accelerating in the opposite direction as currently moving. left
//    else if (player->get_velocity_x() < 0 && x_input > 0)
//    {
//        player->alter_velocity_x(player_consts::X_DEACCELERATION * dt * x_input);
//        player->alter_velocity_x(player_consts::X_ACCELERATION * dt * x_input);
//    }
//    //No key pressed deaccelaration right
//    else if (player->get_velocity_x() > 0 && x_input == 0)
//    {
//        player->alter_velocity_x(-player_consts::X_DEACCELERATION * dt);
//        if (player->get_velocity_x() < 0)
//        {
//            player->set_velocity_x(0.0f);
//        }
//    }
//    //No key pressed deaccelaration left
//    else if (player->get_velocity_x() < 0 && x_input == 0)
//    {
//        player->alter_velocity_x(player_consts::X_DEACCELERATION * dt);
//        if (player->get_velocity_x() > 0)
//        {
//            player->set_velocity_x(0.0f);
//        }
//    }
//    //gravity
//    player->alter_velocity_y(player_consts::GRAVITY * dt);
//    if (player->get_velocity_y() > player_consts::MAX_VELOCITY.y)
//    {
//        player->set_velocity_y(player_consts::MAX_VELOCITY.y);
//    }
//
//    //// for testing only. NOT FOR NORMAL PLAY
//    //player->set_vel_y(0.0f);
//
//
//    this->do_jump(player);
//
//    player->set_dx_x(player->get_velocity_x() * dt);
//    player->set_dx_y(player->get_velocity_y() * dt);
//
//    player->alter_position_x(player->get_dx_x());
//    player->alter_position_y(player->get_dx_y());
//
//    if (x_input > 0.0f)
//    {
//        player->set_facing_right(true);
//    }
//    else if (x_input < 0.0f)
//    {
//        player->set_facing_right(false);
//    }
//}
//
//void PlayerUpdater::do_jump(Player* player)
//{
//    bool req_jump = player->get_input_requesting_jump();
//    float dt = this->get_dt();
//    
//    if (player->get_on_ground())
//    {
//        player->set_air_time(0.0f);
//        player->set_jumping(false);
//    }
//    else if (player->get_on_ceiling())
//    {
//        player->set_jumping(false);
//    }
//    else
//    {
//        player->alter_air_time(dt);
//    }
//    if (req_jump && !player->get_jumping() &&
//        !player->get_prev_requesting_jump() && player->get_on_ground())
//    {
//        player->set_velocity_y(player_consts::JUMP_LAUNCH_VELOCITY *
//            (1 - pow(player->get_air_time() /
//                player_consts::JUMP_MAX_TIME, player_consts::JUMP_POWER)));
//        player->set_jumping(true);
//        player->set_on_ground(false);
//    }
//    else if (req_jump && player->get_jumping())
//    {
//        if (player->get_air_time() <= player_consts::JUMP_MAX_TIME)
//        {
//            player->set_velocity_y(player_consts::JUMP_LAUNCH_VELOCITY *
//                (1 - pow(player->get_air_time() /
//                    player_consts::JUMP_MAX_TIME, player_consts::JUMP_POWER)));
//            player->set_jumping(true);
//            player->set_on_ground(false);
//        }
//        else
//        {
//            player->set_jumping(false);
//        }
//    }
//    else if (!req_jump && player->get_jumping())
//    {
//        player->set_velocity_y(0.0f);
//        player->set_jumping(false);
//        player->set_air_time(player_consts::JUMP_MAX_TIME);
//    }
//    if (player->get_velocity_y() < -player_consts::MAX_VELOCITY.y)
//    {
//        player->set_velocity_y(-player_consts::MAX_VELOCITY.y);
//    }
//    player->set_prev_requesting_jump(req_jump);
//}
//
//void PlayerUpdater::reset(Player* player)
//{
//
//}
//
