#pragma once

#include "engine/render/rotation_origin.h"
#include "engine/math/vector2f.h"

struct ProjectileDetails
{
    float gravity = 0.0f;
    float wind_resistance = 0.0f;
    labrador::RotationOrigin rotat_origin = labrador::RotationOrigin::no_rotation;
    float delete_timer = 0.0f;

    // Health taken off a player this hits, as a positive amount.
    //
    // This number used to exist twice. ProjectileDetails carried one - signed
    // negative, and ranging to -1000.0f for the roller - which nothing ever
    // read: `Projectile::player_damage()` had no callers. What the game
    // actually applied was a second set in player_consts.h, dispatched by a
    // ten-branch if-chain over CollisionObjectType, and the two disagreed by a
    // factor of ten on spray and by five orders of magnitude on rolling. The
    // live numbers are the ones kept; this is now the only place any of them
    // is written, which is the whole point of a number living in data.
    float player_damage = 0.0f;

    mattmath::Vector2F size = { 0.0f, 0.0f };
    mattmath::Vector2F col_rect_size = { 0.0f, 0.0f };
    std::string sheet_name = "";
    std::string animation_strip_name = "";
};

enum class DiffusionType
{
    no_diffusion,
    linear_size,
    linear_size_and_col_rect
};

struct DiffusionDetails
{
    float start_time = 0.0f;
    float end_time = 0.0f;
    mattmath::Vector2F end_scale = { 1.0f, 1.0f };
    DiffusionType type = DiffusionType::no_diffusion;
};
