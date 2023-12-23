#ifndef WEAPON_CONSTS_H
#define WEAPON_CONSTS_H

#include "weapon_details.h"
#include "projectile_consts.h"

namespace weapon_consts
{
    //constexpr float PI = 3.14159265358979323846f;
    constexpr float AMMO_REGEN_DELAY = 2.0f;
    constexpr float AMMO_REGEN_RATE = 0.65f;
    constexpr float STARTING_AMMO = 1.0f;
    const static MattMath::Vector2F NOZZLE_SIZE = { 5.0f, 5.0f };
    
    static const weapon_details DETAILS_DEFAULT =
    {
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        0.0f,
        1100.0f,
        //wep_movement::FREE_ROTATE,     //movement
        //shoot_pattern::SINGLE,           //pattern
        //0.25f,                              //player_vel_amount
        0.0f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "error",                        //frame_name
        projectile_type::SPRAY,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "UI_Clicks01",                  //shoot_sound_name
        1.0f,                           //shoot_sound_volume
        //rotation_origin::LEFT_CENTER,  
    };

    static const relative_weapon_details DETAILS_RELATIVE_DEFAULT =
    {
        add_player_velocity::NONE,
        1.0f,
    };

    static const weapon_details DETAILS_SPRAYER =
    {
        { 0.0f, 0.0f },		//offset
        { 70.0f, 30.0f },	//size
        { projectile_consts::DETAILS_SPRAY.size.x / 2.0f, 0.0f },   //nozzle_offset
        0.0f,						//shoot_interval
        1100.0f,                    //starting_vel_length
        //wep_movement::FREE_ROTATE,    //movement
        //shoot_pattern::SINGLE,           //pattern
        //0.25f,                              //player_vel_amount
        0.007f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "sprayer",                        //frame_name
        projectile_type::SPRAY,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "WaterSprayIntoBucket",                  //shoot_sound_name
        1.0f,                           //shoot_sound_volume
        //rotation_origin::LEFT_CENTER,
    };

    static const relative_weapon_details DETAILS_RELATIVE_SPRAYER =
    {
        add_player_velocity::X_ONLY,
        1.0f,
    };

    static const sound_effect_instance_weapon_details SPRAYER_SOUND_DETAILS =
	{
        "sprayer_a0",
        "sprayer_a1",
        "sprayer_a2",
        "sprayer_a3",
        "sprayer_b0",
        "sprayer_b1",
        "sprayer_b2",
        "sprayer_b3",
	};

    static const weapon_details DETAILS_SNIPER =
    {
        { 20.0f, 0.0f },		//offset
        { 100.0f, 30.0f },	//size
        { projectile_consts::DETAILS_JET.size.x / 2.0f, 0.0f },	//nozzle_offset
        1.0f,						//shoot_interval
        2000.0f,                    //starting_vel_length
        //wep_movement::FREE_ROTATE,     //movement
        //shoot_pattern::SINGLE,           //pattern
        //-1.0f,                              //player_vel_amount
        0.25f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "sniper",                        //frame_name
        projectile_type::JET,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "WoodRattle",                  //shoot_sound_name
        0.8f,                           //shoot_sound_volume
        //rotation_origin::LEFT_CENTER,
    };

    static const weapon_details DETAILS_ROLLER =
    {
        { -20.0f, -10.0 },		//offset
        { 100.0f, 50.0f },	//size
        { -projectile_consts::DETAILS_ROLLING.size.x / 2.0f, 0.0f },	//nozzle_offset
        0.0001f,						//shoot_interval
        0.0f,                        //starting_vel_length
        //wep_movement::ON_GROUND,     //movement
        //shoot_pattern::SINGLE,           //pattern
        //-1.0f,                              //player_vel_amount
        0.004f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "roller",                        //frame_name
        projectile_type::ROLLING,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "BowlingBallRoll",                  //shoot_sound_name
        2.0f,                           //shoot_sound_volume
        //rotation_origin::LEFT_CENTER,
    };

    static const sound_effect_instance_weapon_details ROLLER_SOUND_DETAILS =
    {
        "roller_a0",
        "roller_a1",
        "roller_a2",
        "roller_a3",
        "roller_b0",
        "roller_b1",
        "roller_b2",
        "roller_b3",
    };

    static const weapon_details DETAILS_MISTER =
    {
        { 20.0f, 0.0f },		//offset
        { 60.0f, 30.0f },	//size
        { projectile_consts::DETAILS_MIST.size.x / 2.0f, 0.0f },	//nozzle_offset
        0.1f,						//shoot_interval
        300.0f,                     //starting_vel_length
        //wep_movement::FREE_ROTATE,     //movement
        //shoot_pattern::MIST,           //pattern
        //0.25f,                              //player_vel_amount
        0.01f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "mister",                        //frame_name
        projectile_type::MIST,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "VulcanMachineGunSeries",                  //shoot_sound_name
        0.2f,                           //shoot_sound_volume
        //rotation_origin::LEFT_CENTER,
    };

    static const relative_weapon_details DETAILS_RELATIVE_MISTER =
    {
        add_player_velocity::X_ONLY,
        1.0f,
    };

    static const sound_effect_instance_weapon_details MISTER_SOUND_DETAILS =
    {
        "mister_a0",
        "mister_a1",
        "mister_a2",
        "mister_a3",
        "mister_b0",
        "mister_b1",
        "mister_b2",
        "mister_b3",
    };

    static const weapon_details DETAILS_BUCKET =
    {
        { 10.0f, -10.0f },		//offset
        { 60.0f, 60.0f },	//size
        { projectile_consts::DETAILS_BALL.size.x / 2.0f, 0.0f },	//nozzle_offset
        0.5f,						//shoot_interval
        800.0f,                     //starting_vel_length
        //wep_movement::FREE_ROTATE,     //movement
        //shoot_pattern::MIST,           //pattern
        //0.25f,                              //player_vel_amount
        0.1f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "bucket",                        //frame_name
        projectile_type::BALL,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "LaserGun",                  //shoot_sound_name
        0.3f,                           //shoot_sound_volume
        //rotation_origin::LEFT_CENTER,
    };

    static const relative_weapon_details DETAILS_RELATIVE_BUCKET =
    {
        add_player_velocity::X_ONLY,
        1.0f,
    };
}

#endif // !WEAPON_CONSTS_H
