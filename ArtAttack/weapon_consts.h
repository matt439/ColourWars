#ifndef WEAPON_CONSTS_H
#define WEAPON_CONSTS_H

#include "WeaponDetails.h"
#include "projectile_consts.h"

namespace weapon_consts
{
    constexpr float AMMO_REGEN_DELAY = 0.5f;
    constexpr float AMMO_REGEN_RATE = 0.4f;
    constexpr float STARTING_AMMO = 1.0f;
    const static MattMath::Vector2F NOZZLE_SIZE = { 5.0f, 5.0f };
    
    static const WeaponDetails DETAILS_DEFAULT =
    {
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        0.0f,
        1100.0f,
        0.0f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "error",                        //frame_name
        SPRAY,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "UI_Clicks01",                  //shoot_sound_name
        1.0f,                           //shoot_sound_volume
    };

    static constexpr RelativeWeaponDetails DETAILS_RELATIVE_DEFAULT =
    {
        add_player_velocity::NONE,
        1.0f,
    };

    static const WeaponDetails DETAILS_SPRAYER =
    {
        { 0.0f, 0.0f },		//offset
        { 70.0f, 30.0f },	//size
        { projectile_consts::DETAILS_SPRAY.size.x / 2.0f, 0.0f },   //nozzle_offset
        0.0f,						//shoot_interval
        1100.0f,                    //starting_vel_length
        0.007f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "sprayer",                        //frame_name
        SPRAY,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "WaterSprayIntoBucket",                  //shoot_sound_name
        1.0f,                           //shoot_sound_volume
    };

    static constexpr RelativeWeaponDetails DETAILS_RELATIVE_SPRAYER =
    {
        add_player_velocity::X_ONLY,
        1.0f,
    };

    static const SoundEffectInstanceWeaponDetails SPRAYER_SOUND_DETAILS =
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

    static const WeaponDetails DETAILS_SNIPER =
    {
        { 15.0f, -20.0f },		//offset
        { 100.0f, 30.0f },	//size
        { projectile_consts::DETAILS_JET.size.x / 2.0f, 0.0f },	//nozzle_offset
        1.0f,						//shoot_interval
        2000.0f,                    //starting_vel_length
        0.25f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "sniper",                        //frame_name
        JET,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "WoodRattle",                  //shoot_sound_name
        0.8f,                           //shoot_sound_volume
    };

    static const WeaponDetails DETAILS_ROLLER =
    {
        { -20.0f, -10.0 },		//offset
        { 100.0f, 50.0f },	//size
        { -projectile_consts::DETAILS_ROLLING.size.x / 2.0f, 0.0f },	//nozzle_offset
        0.0001f,						//shoot_interval
        0.0f,                        //starting_vel_length
        0.004f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "roller",                        //frame_name
    	ROLLING,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "BowlingBallRoll",                  //shoot_sound_name
        3.0f,                           //shoot_sound_volume
    };

    static const SoundEffectInstanceWeaponDetails ROLLER_SOUND_DETAILS =
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

    static const WeaponDetails DETAILS_MISTER =
    {
        { 20.0f, 0.0f },		//offset
        { 60.0f, 30.0f },	//size
        { projectile_consts::DETAILS_MIST.size.x / 2.0f, 0.0f },	//nozzle_offset
        0.1f,						//shoot_interval
        300.0f,                     //starting_vel_length
        0.01f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "mister",                        //frame_name
        MIST,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "VulcanMachineGunSeries",                  //shoot_sound_name
        0.2f,                           //shoot_sound_volume
    };

    static constexpr RelativeWeaponDetails DETAILS_RELATIVE_MISTER =
    {
        add_player_velocity::X_ONLY,
        1.0f,
    };

    static const SoundEffectInstanceWeaponDetails MISTER_SOUND_DETAILS =
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

    static const WeaponDetails DETAILS_BUCKET =
    {
        { 10.0f, -10.0f },		//offset
        { 60.0f, 60.0f },	//size
        { projectile_consts::DETAILS_BALL.size.x / 2.0f, 0.0f },	//nozzle_offset
        0.5f,						//shoot_interval
        800.0f,                     //starting_vel_length
        0.1f,                              //ammo_usage
        "sprite_sheet_1",               //sheet_name
        "bucket",                        //frame_name
        BALL,         //projectile_type
        "sound_bank_1",                 //sound_bank_name
        "LaserGun",                  //shoot_sound_name
        0.3f,                           //shoot_sound_volume
    };

    static constexpr RelativeWeaponDetails DETAILS_RELATIVE_BUCKET =
    {
        add_player_velocity::X_ONLY,
        1.0f,
    };
}

#endif // !WEAPON_CONSTS_H
