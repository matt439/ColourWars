#pragma once

#include "engine/math/quad.h"
#include "engine/math/rectangle_rotated.h"
#include "engine/math/rectanglef.h"
#include "engine/math/segment.h"
#include <optional>

enum class FixedSlabRise { left, right };

struct FixedSlabSpec
{
    // The endpoint with the smaller world X, not the smaller Y. Both top
    // endpoints must remain finite and within +/-65536 on each world axis.
    mattmath::Vector2F top_left = mattmath::Vector2F::ZERO;
    FixedSlabRise rise = FixedSlabRise::right;
};

struct FixedSlabSweep
{
    double entry = 0.0;
    double exit = 1.0;
    // From the slab towards the swept rectangle at entry. Initial overlap
    // instead reports the minimum-translation direction at the initial pose.
    mattmath::Vector2F normal_to_rectangle = mattmath::Vector2F::ZERO;
    bool initially_overlapping = false;
    // Closed SAT also admits pure touching (including a whole step along an
    // edge). Such an interval must not create projectile paint or damage.
    bool positive_overlap = false;
};

// One fixed-size, fixed-orientation solid. Local X follows the top from left
// to right; local Y points inward through its 24-unit normal thickness.
// This is not a general rotating body or a moving-platform extension.
class FixedSlabGeometry final
{
public:
    static constexpr float horizontal_span = 160.0f;
    static constexpr float vertical_rise = 80.0f;
    static constexpr float normal_thickness = 24.0f;

    explicit FixedSlabGeometry(const FixedSlabSpec& spec);
    const FixedSlabSpec& spec() const;
    const mattmath::RectangleRotated& shape() const;
    mattmath::RectangleF bounds() const;
    mattmath::Vector2F top_start() const;
    mattmath::Vector2F top_end() const;
    mattmath::Segment top() const;
    mattmath::Vector2F tangent() const;
    mattmath::Vector2F inward_normal() const;
    float length() const;
    mattmath::RectangleF local_rectangle() const;
    mattmath::Vector2F to_world(const mattmath::Vector2F& local) const;
    mattmath::Vector2F to_local(const mattmath::Vector2F& world) const;
    mattmath::Quad world_quad(const mattmath::RectangleF& local) const;
    // Preserve the actual rotated footprint: using its inverse AABB would
    // paint tiles outside a diagonal projectile's footprint.
    mattmath::Quad local_quad(const mattmath::RectangleF& world) const;

    bool overlaps(const mattmath::RectangleF& rectangle) const;
    // Each of the four rectangle bounds interpolates linearly. This handles
    // the current growing Sprayer/Mister boxes as well as rigid translation.
    // Includes closed endpoint/tangent intervals; consult positive_overlap.
    std::optional<FixedSlabSweep> sweep(const mattmath::RectangleF& start,
        const mattmath::RectangleF& finish) const;

private:
    FixedSlabSpec spec_;
    mattmath::RectangleRotated shape_;
};
