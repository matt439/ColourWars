#pragma once

#include "game/paint_face_geometry.h"
#include "game/paint_polygon_geometry.h"
#include <cmath>
#include <limits>
#include <stdexcept>

// The authored top endpoints own static face adjacency. Reconstructing an OBB
// from its centre/normalized axes can move these joins by a few float ULPs and
// invent a paintable sliver. Correct only that known reconstruction here: no
// obstacle coordinate is rounded, and the actual collider/drawn tile pose stays
// unchanged. Four coordinate ULPs bound the centre/axis multiply-add sequence;
// a different footprint or larger correction is an error, not an area waiver.
inline PaintPolygon canonical_slab_paint_polygon(const PaintPolygon& represented,
    PaintPoint authored_start, PaintPoint authored_end, PaintPoint inward)
{
    const auto as_float = [](double value)
    {
        const float result = static_cast<float>(value);
        if (!std::isfinite(result)) { throw std::invalid_argument("Fixed slab paint coordinates must be finite"); }
        return result;
    };
    const float x0 = as_float(authored_start.x), y0 = as_float(authored_start.y);
    const float x1 = as_float(authored_end.x), y1 = as_float(authored_end.y);
    const float nx = as_float(inward.x), ny = as_float(inward.y);
    if (x1 - x0 != 160.0f || std::abs(y1 - y0) != 80.0f || !(ny > 0.0f))
    { throw std::invalid_argument("Fixed slab paint requires the selected authored footprint and inward axis"); }
    const PaintPolygon result{ PaintPoint{ x0, y0 }, PaintPoint{ x1, y1 },
        PaintPoint{ x1 + nx * 24.0f, y1 + ny * 24.0f },
        PaintPoint{ x0 + nx * 24.0f, y0 + ny * 24.0f } };
    for (std::size_t index = 0; index < result.size(); ++index)
    {
        const auto checked = [&](double actual, double canonical)
        {
            const float old_value = as_float(actual), new_value = as_float(canonical);
            const float scale = std::max({ 160.0f, std::abs(old_value), std::abs(new_value) });
            const float ulp = std::nextafter(scale, std::numeric_limits<float>::infinity()) - scale;
            if (std::abs(actual - canonical) > 4.0 * ulp)
            { throw std::invalid_argument("Fixed slab paint reconstruction exceeds four coordinate ULPs"); }
        };
        checked(represented[index].x, result[index].x);
        checked(represented[index].y, result[index].y);
    }
    return result;
}

// Exposed intervals are measured in the unchanged local face units. Use the
// canonical authored adjacency polygon for clipping in both runtime/editor.
inline std::vector<std::pair<float, float>> exposed_slab_paint_edge(const PaintPolygon& polygon,
    float extent, bool along_x, bool leading, std::span<const PaintFaceRectangle> obstacles)
{
    const std::size_t begin_index = along_x ? (leading ? 0u : 3u) : (leading ? 0u : 1u);
    const std::size_t end_index = along_x ? (leading ? 1u : 2u) : (leading ? 3u : 2u);
    const PaintPoint start = polygon[begin_index], finish = polygon[end_index];
    const PaintPoint direction{ (finish.x - start.x) / extent, (finish.y - start.y) / extent };
    const PaintPoint top{ polygon[1].x - polygon[0].x, polygon[1].y - polygon[0].y };
    const PaintPoint normal{ polygon[3].x - polygon[0].x, polygon[3].y - polygon[0].y };
    const double sign = leading ? -1.0 : 1.0;
    const PaintPoint outside = along_x ? PaintPoint{ normal.x * sign, normal.y * sign }
        : PaintPoint{ top.x * sign, top.y * sign };
    std::vector<std::pair<double, double>> covered;
    for (const auto& obstacle : obstacles)
    {
        if (!std::isfinite(obstacle.left) || !std::isfinite(obstacle.right) ||
            !std::isfinite(obstacle.top) || !std::isfinite(obstacle.bottom) ||
            !(obstacle.right > obstacle.left && obstacle.bottom > obstacle.top))
        { throw std::invalid_argument("A fixed slab needs finite positive paint obstacles"); }
        double first = 0.0, last = extent;
        const auto clip_axis = [&](double origin, double rate, double low, double high)
        {
            if (rate == 0.0) { return origin >= low && origin <= high; }
            double a = (low - origin) / rate, b = (high - origin) / rate;
            if (a > b) { std::swap(a, b); }
            first = std::max(first, a); last = std::min(last, b);
            return first < last;
        };
        if (!clip_axis(start.x, direction.x, obstacle.left, obstacle.right) ||
            !clip_axis(start.y, direction.y, obstacle.top, obstacle.bottom) ||
            !(first < last)) { continue; }
        const double middle = first + (last - first) / 2.0;
        const auto enters = [](double point, double low, double high, double outward)
        {
            return (point > low && point < high) ||
                (point == low && outward > 0.0) || (point == high && outward < 0.0);
        };
        if (enters(start.x + direction.x * middle, obstacle.left, obstacle.right, outside.x) &&
            enters(start.y + direction.y * middle, obstacle.top, obstacle.bottom, outside.y))
        { covered.emplace_back(first, last); }
    }
    std::sort(covered.begin(), covered.end());
    std::vector<std::pair<float, float>> result;
    double cursor = 0.0;
    for (const auto& interval : covered)
    {
        if (cursor < interval.first)
        { result.emplace_back(static_cast<float>(cursor), static_cast<float>(interval.first)); }
        cursor = std::max(cursor, interval.second);
    }
    if (cursor < extent) { result.emplace_back(static_cast<float>(cursor), extent); }
    return result;
}
