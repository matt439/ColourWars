#ifndef PROJECTILE_CONSTS_H
#define PROJECTILE_CONSTS_H

#include "projectile_details.h"

namespace projectile_consts
{
    const MattMath::Vector2F DEFAULT_STARTING_POSITION = { 0.0f, 0.0f };
    const MattMath::Vector2F DEFAULT_SIZE = { 10.0f, 10.0f };
    const MattMath::Vector2F STARTING_DX = { 0.0f, 0.0f };
    const MattMath::Vector2F STARTING_VELOCITY = { 0.0f, 0.0f };
    const MattMath::Vector2F MAX_VELOCITY = { 5000.0f, 5000.0f };

    static const projectile_details DETAILS_DEFAULT =
    {
        2500.0f,                        //gravity
        20.0f,                          //wind_resistance
        rotation_origin::NO_ROTATION,   //rotat_origin
        1.0f,                           //delete_timer
        //0.0f,                           //colour_power
        0.0f,                           //player_damage
        MattMath::Vector2F(15.0f, 15.0f),        //size
        MattMath::Vector2F(15.0f, 15.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "test",                        //animation_strip_name
    };

    static const projectile_details DETAILS_SPRAY =
    {
        2500.0f,                        //gravity
        20.0f,                          //wind_resistance
        rotation_origin::NO_ROTATION,   //rotat_origin
        5.0f,                           //delete_timer
        //1.0f,                          //colour_power
        -0.1f,                           //player_damage
        MattMath::Vector2F(15.0f, 15.0f),        //size
        MattMath::Vector2F(15.0f, 15.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const diffusion_details DIFFUSION_DETAILS_SPRAY =
    {
        0.2f,      //start_time
        1.0f,      //end_time
        1.5f,      //end_size
        diffusion_type::LINEAR_SIZE_AND_COL_RECT, //diffusion_type
    };

    static const projectile_details DETAILS_JET =
    {
        0.0f,                           //gravity
        0.0f,                           //wind_resistance
        rotation_origin::CENTER,        //rotat_origin
        5.0f,                           //delete_timer
        //1000.0f,                        //colour_power
        -0.5f,                           //player_damage
        MattMath::Vector2F(20.0f, 20.0f),         //size
        MattMath::Vector2F(20.0f, 20.0f),           //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const projectile_details DETAILS_ROLLING =
    {
        0.0f,                           //gravity
        0.0f,                           //wind_resistance
        rotation_origin::NO_ROTATION,   //rotat_origin
        0.000001f,                           //delete_timer
        //1000.0f,                        //colour_power
        -1000.0f,                        //player_damage
        MattMath::Vector2F(50.0f, 50.0f),        //size
        MattMath::Vector2F(50.0f, 50.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const projectile_details DETAILS_MIST =
    {
        0.0f,                        //gravity
        0.0f,                          //wind_resistance
        rotation_origin::NO_ROTATION,   //rotat_origin
        5.0f,                           //delete_timer
        //5.0f,                          //colour_power
        -0.05f,                           //player_damage
        MattMath::Vector2F(5.0f, 5.0f),        //size
        MattMath::Vector2F(5.0f, 5.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const diffusion_details DIFFUSION_DETAILS_MIST =
    {
        0.2f,      //start_time
        2.0f,      //end_time
        5.0f,      //end_size
        diffusion_type::LINEAR_SIZE_AND_COL_RECT,
    };

    static const projectile_details DETAILS_BALL =
    {
        2500.0f,                        //gravity
        20.0f,                          //wind_resistance
        rotation_origin::NO_ROTATION,   //rotat_origin
        5.0f,                           //delete_timer
        //100.0f,                          //colour_power
        -0.1f,                           //player_damage
        MattMath::Vector2F(100.0f, 100.0f),        //size
        MattMath::Vector2F(100.0f, 100.0f),         //col_rect_size.
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };
}

#endif // !PROJECTILE_CONSTS_H
