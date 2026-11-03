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
    labrador::Vector2F top_left = labrador::Vector2F::zero;
    FixedSlabRise rise = FixedSlabRise::right;
};

struct FixedSlabSweep
{
    double entry = 0.0;
    double exit = 1.0;
    // From the slab towards the swept rectangle at entry. Initial overlap
    // instead reports the minimum-translation direction at the initial pose.
    labrador::Vector2F normal_to_rectangle = labrador::Vector2F::zero;
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
    const labrador::RectangleRotated& shape() const;
    labrador::RectangleF bounds() const;
    labrador::Vector2F top_start() const;
    labrador::Vector2F top_end() const;
    labrador::Segment top() const;
    labrador::Vector2F tangent() const;
    labrador::Vector2F inward_normal() const;
    float length() const;
    labrador::RectangleF local_rectangle() const;
    labrador::Vector2F to_world(const labrador::Vector2F& local) const;
    labrador::Vector2F to_local(const labrador::Vector2F& world) const;
    labrador::Quad world_quad(const labrador::RectangleF& local) const;
    // Preserve the actual rotated footprint: using its inverse AABB would
    // paint tiles outside a diagonal projectile's footprint.
    labrador::Quad local_quad(const labrador::RectangleF& world) const;

    bool overlaps(const labrador::RectangleF& rectangle) const;
    // Each of the four rectangle bounds interpolates linearly. This handles
    // the current growing Sprayer/Mister boxes as well as rigid translation.
    // Includes closed endpoint/tangent intervals; consult positive_overlap.
    std::optional<FixedSlabSweep> sweep(const labrador::RectangleF& start,
        const labrador::RectangleF& finish) const;

private:
    FixedSlabSpec spec_;
    labrador::RectangleRotated shape_;
};
