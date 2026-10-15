#pragma once

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

// Shared by runtime paint tiles and the editor's scoring diagnostics. Only flat
// structures (including jump-through structures) belong in the obstacle list.
struct PaintFaceRectangle
{
    float left, top, right, bottom;
};

inline std::vector<std::pair<float, float>> exposed_paint_intervals(
    float low, float high, float edge, bool along_x, bool leading,
    std::span<const PaintFaceRectangle> obstacles)
{
    std::vector<std::pair<float, float>> covered;
    for (const PaintFaceRectangle& obstacle : obstacles)
    {
        const float near_edge = along_x ? obstacle.top : obstacle.left;
        const float far_edge = along_x ? obstacle.bottom : obstacle.right;
        // Only coverage reaching the face from outside buries it. The owning
        // rectangle and a neighbour behind the face must not remove its score.
        const bool covers = leading ? near_edge < edge && far_edge >= edge
            : near_edge <= edge && far_edge > edge;
        const float begin = std::max(low, along_x ? obstacle.left : obstacle.top);
        const float end = std::min(high, along_x ? obstacle.right : obstacle.bottom);
        if (covers && begin < end) covered.emplace_back(begin, end);
    }
    std::sort(covered.begin(), covered.end());
    std::vector<std::pair<float, float>> exposed;
    float cursor = low;
    for (const std::pair<float, float>& interval : covered)
    {
        if (cursor < interval.first) exposed.emplace_back(cursor, interval.first);
        cursor = std::max(cursor, interval.second);
    }
    if (cursor < high) exposed.emplace_back(cursor, high);
    return exposed;
}
