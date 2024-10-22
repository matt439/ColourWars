#ifndef PROJECTILE_DETAILS_H
#define PROJECTILE_DETAILS_H

#include "rotation_origin.h"
#include "MattMath.h"

struct ProjectileDetails
{
    float gravity = -1.0f;
    float wind_resistance = -1.0f;
    rotation_origin rotat_origin = rotation_origin::NO_ROTATION;
    float delete_timer = -1.0f;
    float player_damage = -1.0f;
    MattMath::Vector2F size = { 0.0f, 0.0f };
    MattMath::Vector2F col_rect_size = { 0.0f, 0.0f };
    std::string sheet_name = "";
    std::string animation_strip_name = "";
};

enum class diffusion_type
{
    NO_DIFFUSION,
    LINEAR_SIZE,
    LINEAR_SIZE_AND_COL_RECT
};

struct DiffusionDetails
{
    float start_time = -1.0f;
    float end_time = -1.0f;
    MattMath::Vector2F end_scale = { -1.0f, -1.0f };
    diffusion_type type = diffusion_type::NO_DIFFUSION;
};

#endif // !PROJECTILE_DETAILS_H
