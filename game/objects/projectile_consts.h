#pragma once

#include "game/objects/projectile_details.h"

namespace projectile_consts
{
    const mattmath::Vector2F DEFAULT_STARTING_POSITION = { 0.0f, 0.0f };
    const mattmath::Vector2F DEFAULT_SIZE = { 10.0f, 10.0f };
    const mattmath::Vector2F STARTING_DX = { 0.0f, 0.0f };
    const mattmath::Vector2F STARTING_VELOCITY = { 0.0f, 0.0f };
    const mattmath::Vector2F MAX_VELOCITY = { 5000.0f, 5000.0f };

    static const ProjectileDetails DETAILS_DEFAULT =
    {
        2500.0f,                        //gravity
        20.0f,                          //wind_resistance
        RotationOrigin::no_rotation,   //rotat_origin
        1.0f,                           //delete_timer
        0.0f,                           //player_damage
        mattmath::Vector2F(15.0f, 15.0f),        //size
        mattmath::Vector2F(15.0f, 15.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "test",                        //animation_strip_name
    };

    static const ProjectileDetails DETAILS_SPRAY =
    {
        2500.0f,                        //gravity
        20.0f,                          //wind_resistance
        RotationOrigin::no_rotation,   //rotat_origin
        5.0f,                           //delete_timer
        -0.1f,                           //player_damage
        mattmath::Vector2F(15.0f, 15.0f),        //size
        mattmath::Vector2F(15.0f, 15.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const DiffusionDetails DIFFUSION_DETAILS_SPRAY =
    {
        0.2f,      //start_time
        1.0f,      //end_time
        1.5f,      //end_size
        DiffusionType::linear_size_and_col_rect, //DiffusionType
    };

    static const ProjectileDetails DETAILS_JET =
    {
        0.0f,                           //gravity
        0.0f,                           //wind_resistance
        RotationOrigin::center,        //rotat_origin
        5.0f,                           //delete_timer
        -0.5f,                           //player_damage
        mattmath::Vector2F(20.0f, 20.0f),         //size
        mattmath::Vector2F(20.0f, 20.0f),           //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const ProjectileDetails DETAILS_ROLLING =
    {
        0.0f,                           //gravity
        0.0f,                           //wind_resistance
        RotationOrigin::no_rotation,   //rotat_origin
        0.000001f,                           //delete_timer
        -1000.0f,                        //player_damage
        mattmath::Vector2F(50.0f, 50.0f),        //size
        mattmath::Vector2F(50.0f, 50.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const ProjectileDetails DETAILS_MIST =
    {
        0.0f,                        //gravity
        0.0f,                          //wind_resistance
        RotationOrigin::no_rotation,   //rotat_origin
        5.0f,                           //delete_timer
        -0.05f,                           //player_damage
        mattmath::Vector2F(5.0f, 5.0f),        //size
        mattmath::Vector2F(5.0f, 5.0f),         //col_rect_size
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };

    static const DiffusionDetails DIFFUSION_DETAILS_MIST =
    {
        0.2f,      //start_time
        2.0f,      //end_time
        5.0f,      //end_size
        DiffusionType::linear_size_and_col_rect,
    };

    static const ProjectileDetails DETAILS_BALL =
    {
        2500.0f,                        //gravity
        20.0f,                          //wind_resistance
        RotationOrigin::no_rotation,   //rotat_origin
        5.0f,                           //delete_timer
        -0.1f,                           //player_damage
        mattmath::Vector2F(100.0f, 100.0f),        //size
        mattmath::Vector2F(100.0f, 100.0f),         //col_rect_size.
        "sprite_sheet_1",               //sheet_name
        "projectile_3",                        //animation_strip_name
    };
}
