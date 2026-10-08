#pragma once

#include "engine/render/rotation_origin.h"
#include "engine/math/vector2f.h"

struct ProjectileDetails
{
    float gravity = 0.0f;
    float wind_resistance = 0.0f;
    labrador::RotationOrigin rotat_origin = labrador::RotationOrigin::no_rotation;
    float delete_timer = 0.0f;

    // Health removed on a hit, expressed as a positive amount.
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
