#pragma once

#include <cstddef>
#include <vector>

// Authored/local face identity; a rigid pose may change its world normal.
enum class PaintSurfaceFace { top, bottom, left, right };

// Indices refer to the level's collision array, in its authored order.
struct PaintFillRegion
{
    std::size_t trigger_object;
    std::vector<PaintSurfaceFace> trigger_faces;
    std::size_t target_object;
    PaintSurfaceFace target_face;
};
