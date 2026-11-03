#pragma once

// Carried from Labrador v0.2.0, which removes these members in v0.3.0
// (fdfc24e, "keep a copy of the removed function in the game"). Each body is
// the engine's, unchanged; as free functions in colourwars:: they can never be
// ambiguous with an engine name. See docs/review/engine-reports.md, report 28.

#include "engine/math/rectanglef.h"
#include "engine/math/scalar.h"
#include "engine/math/vector2f.h"
#include "engine/render/camera.h"

namespace colourwars
{
    // Was RectangleF::from_top_left_bottom_right(const Vector2F&, const Vector2F&).
    inline labrador::RectangleF from_top_left_bottom_right(const labrador::Vector2F& top_left,
        const labrador::Vector2F& bottom_right)
    {
        return labrador::RectangleF(top_left.x, top_left.y,
            bottom_right.x - top_left.x,
            bottom_right.y - top_left.y);
    }

    // Was Camera::calculate_intermediate_camera.
    inline labrador::Camera intermediate_camera(const labrador::Camera& first,
        const labrador::Camera& last, float amount)
    {
        return labrador::Camera(labrador::Vector2F::lerp(first.translation, last.translation, amount),
            labrador::lerp(first.scale, last.scale, amount));
    }
}
