#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <utility>
#include <vector>

struct PaintPoint { double x = 0.0, y = 0.0; };
// Convex corners, in top-left/top-right/bottom-right/bottom-left winding.
using PaintPolygon = std::array<PaintPoint, 4>;

// Remove only the actual polygon intersection along a face. In particular a
// diagonal solid's empty bounding-box corner must never bury flat paint.
// Empty polygons retain the original intervals and float arithmetic exactly.
inline std::vector<std::pair<float, float>> clip_paint_polygons(
    const std::vector<std::pair<float, float>>& intervals, PaintPoint origin,
    PaintPoint direction, std::span<const PaintPolygon> polygons)
{
    auto result = intervals;
    for (const auto& polygon : polygons)
    {
        std::vector<std::pair<float, float>> next;
        for (const auto& [begin, end] : result)
        {
            double low = begin, high = end;
            bool intersects = true;
            for (std::size_t i = 0; i < polygon.size(); ++i)
            {
                const auto& a = polygon[i];
                const auto& b = polygon[(i + 1) % polygon.size()];
                const double dx = b.x - a.x, dy = b.y - a.y;
                const double value = dx * (origin.y - a.y) - dy * (origin.x - a.x);
                const double change = dx * direction.y - dy * direction.x;
                if (change == 0.0)
                {
                    // Merely coincident/tangent edges do not bury a face.
                    if (value <= 0.0) { intersects = false; break; }
                }
                else if (change > 0.0) { low = std::max(low, -value / change); }
                else { high = std::min(high, -value / change); }
                if (low >= high) { intersects = false; break; }
            }
            if (!intersects) { next.emplace_back(begin, end); continue; }
            const float covered_begin = std::clamp(static_cast<float>(low), begin, end);
            const float covered_end = std::clamp(static_cast<float>(high), begin, end);
            if (begin < covered_begin) { next.emplace_back(begin, covered_begin); }
            if (covered_end < end) { next.emplace_back(covered_end, end); }
        }
        result = std::move(next);
    }
    return result;
}
