#include "game/pch.h"
#include "game/objects/fixed_slab_geometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace mattmath;

namespace
{
    bool finite(const Vector2F& point)
    { return std::isfinite(point.x) && std::isfinite(point.y); }

    void validate_rectangle(const RectangleF& rectangle)
    {
        if (!finite(rectangle.top_left()) || !finite(rectangle.bottom_right()) ||
            !std::isfinite(rectangle.width) || !std::isfinite(rectangle.height) ||
            !(rectangle.width > 0.0f) || !(rectangle.height > 0.0f) ||
            !(rectangle.right() > rectangle.left()) || !(rectangle.bottom() > rectangle.top()))
        { throw std::invalid_argument("Fixed slab requires a finite noncollapsed positive rectangle."); }
    }

    RectangleRotated make_shape(const FixedSlabSpec& spec)
    {
        if (!finite(spec.top_left) || std::abs(spec.top_left.x) > 65536.0f || std::abs(spec.top_left.y) > 65536.0f ||
            (spec.rise != FixedSlabRise::left && spec.rise != FixedSlabRise::right))
        { throw std::invalid_argument("Fixed slab requires a finite endpoint and a supported fixed rise."); }
        const float dy = spec.rise == FixedSlabRise::right
            ? -FixedSlabGeometry::vertical_rise : FixedSlabGeometry::vertical_rise;
        const Vector2F end(spec.top_left.x + FixedSlabGeometry::horizontal_span, spec.top_left.y + dy);
        // At coordinates too large to retain this footprint the rounded box
        // could be narrower, flatter or degenerate despite finite input.
        if (!finite(end) || std::abs(end.x) > 65536.0f || std::abs(end.y) > 65536.0f ||
            end.x - spec.top_left.x != FixedSlabGeometry::horizontal_span ||
            end.y - spec.top_left.y != dy)
        { throw std::invalid_argument("Fixed slab endpoint precision cannot represent its fixed footprint."); }
        const float length = std::hypot(FixedSlabGeometry::horizontal_span, dy);
        const Vector2F tangent(FixedSlabGeometry::horizontal_span / length, dy / length);
        const Vector2F inward(-tangent.y, tangent.x);
        const Vector2F center = spec.top_left + Vector2F(FixedSlabGeometry::horizontal_span / 2.0f, dy / 2.0f) +
            inward * (FixedSlabGeometry::normal_thickness / 2.0f);
        if (!finite(center)) { throw std::invalid_argument("Fixed slab center is not finite."); }
        RectangleRotated result(center, tangent, inward,
            { length / 2.0f, FixedSlabGeometry::normal_thickness / 2.0f });
        for (const auto& point : result.points())
        { if (!finite(point)) { throw std::invalid_argument("Fixed slab corner is not finite."); } }
        const auto& points = result.points();
        if (!(points[1].x > points[0].x) || points[1].y == points[0].y ||
            points[2] == points[1] || points[3] == points[0] || !result.is_valid())
        { throw std::invalid_argument("Fixed slab precision collapsed its top or normal thickness."); }
        return result;
    }

    struct Projection { double minimum; double maximum; };

    Projection project(const RectangleF& rectangle, const Vector2F& axis)
    {
        const double left = rectangle.left(), right = rectangle.right();
        const double top = rectangle.top(), bottom = rectangle.bottom();
        return {
            axis.x * (axis.x >= 0.0f ? left : right) + axis.y * (axis.y >= 0.0f ? top : bottom),
            axis.x * (axis.x >= 0.0f ? right : left) + axis.y * (axis.y >= 0.0f ? bottom : top)
        };
    }

    Projection project(const RectangleRotated& slab, const Vector2F& axis)
    {
        Projection result{ std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity() };
        for (const auto& point : slab.points())
        {
            const double value = static_cast<double>(point.x) * axis.x + static_cast<double>(point.y) * axis.y;
            result.minimum = std::min(result.minimum, value);
            result.maximum = std::max(result.maximum, value);
        }
        return result;
    }

    std::array<Vector2F, 4> axes(const RectangleRotated& slab)
    { return { Vector2F(1.0f, 0.0f), Vector2F(0.0f, 1.0f), slab.x_axis(), slab.y_axis() }; }

    // Intersect one affine half-space g(t)>=0 with the current closed time
    // interval. The normal points from the slab into the rectangle on entry.
    bool clip(double before, double after, const Vector2F& normal, FixedSlabSweep& result)
    {
        if (before < 0.0 && after < 0.0) { return false; }
        if (before >= 0.0 && after >= 0.0)
        {
            if (before == 0.0 && after > 0.0 && result.entry == 0.0 &&
                result.normal_to_rectangle == Vector2F::ZERO)
            { result.normal_to_rectangle = normal; }
            return true;
        }
        const double crossing = -before / (after - before);
        if (before < 0.0)
        {
            if (crossing > result.entry)
            { result.entry = crossing; result.normal_to_rectangle = normal; }
        }
        else { result.exit = std::min(result.exit, crossing); }
        return result.entry <= result.exit;
    }
}

FixedSlabGeometry::FixedSlabGeometry(const FixedSlabSpec& spec) : spec_(spec), shape_(make_shape(spec)) {}
const FixedSlabSpec& FixedSlabGeometry::spec() const { return this->spec_; }
const RectangleRotated& FixedSlabGeometry::shape() const { return this->shape_; }
RectangleF FixedSlabGeometry::bounds() const { return this->shape_.bounding_box(); }
Vector2F FixedSlabGeometry::top_start() const { return this->shape_.point_0(); }
Vector2F FixedSlabGeometry::top_end() const { return this->shape_.point_1(); }
Segment FixedSlabGeometry::top() const { return { this->top_start(), this->top_end() }; }
Vector2F FixedSlabGeometry::tangent() const { return this->shape_.x_axis(); }
Vector2F FixedSlabGeometry::inward_normal() const { return this->shape_.y_axis(); }
float FixedSlabGeometry::length() const { return this->shape_.half_x_width() * 2.0f; }
RectangleF FixedSlabGeometry::local_rectangle() const { return { 0.0f, 0.0f, this->length(), normal_thickness }; }

Vector2F FixedSlabGeometry::to_world(const Vector2F& local) const
{
    if (!finite(local)) { throw std::invalid_argument("Fixed slab local point is not finite."); }
    // The same center/axis order as RectangleRotated's cached corners, so all
    // four full local corners have exactly the collider's represented pose.
    const Vector2F result = this->shape_.center() + this->tangent() * (local.x - this->length() / 2.0f) +
        this->inward_normal() * (local.y - normal_thickness / 2.0f);
    if (!finite(result)) { throw std::invalid_argument("Fixed slab transformed point is not finite."); }
    return result;
}

Vector2F FixedSlabGeometry::to_local(const Vector2F& world) const
{
    if (!finite(world)) { throw std::invalid_argument("Fixed slab world point is not finite."); }
    const double dx = static_cast<double>(world.x) - this->shape_.center().x;
    const double dy = static_cast<double>(world.y) - this->shape_.center().y;
    const Vector2F tangent = this->tangent(), normal = this->inward_normal();
    const double norm = static_cast<double>(tangent.x) * tangent.x + static_cast<double>(tangent.y) * tangent.y;
    const Vector2F result(static_cast<float>((dx * tangent.x + dy * tangent.y) / norm + this->length() / 2.0),
        static_cast<float>((dx * normal.x + dy * normal.y) / norm + normal_thickness / 2.0));
    if (!finite(result)) { throw std::invalid_argument("Fixed slab inverse point is not finite."); }
    return result;
}

Quad FixedSlabGeometry::world_quad(const RectangleF& local) const
{
    validate_rectangle(local);
    return { this->to_world(local.top_left()), this->to_world(local.top_right()),
        this->to_world(local.bottom_right()), this->to_world(local.bottom_left()) };
}

Quad FixedSlabGeometry::local_quad(const RectangleF& world) const
{
    validate_rectangle(world);
    return { this->to_local(world.top_left()), this->to_local(world.top_right()),
        this->to_local(world.bottom_right()), this->to_local(world.bottom_left()) };
}

bool FixedSlabGeometry::overlaps(const RectangleF& rectangle) const
{
    validate_rectangle(rectangle);
    for (const Vector2F& axis : axes(this->shape_))
    {
        const Projection slab = project(this->shape_, axis), box = project(rectangle, axis);
        if (box.maximum <= slab.minimum || box.minimum >= slab.maximum) { return false; }
    }
    return true;
}

std::optional<FixedSlabSweep> FixedSlabGeometry::sweep(const RectangleF& start, const RectangleF& finish) const
{
    validate_rectangle(start); validate_rectangle(finish);
    FixedSlabSweep result;
    result.initially_overlapping = this->overlaps(start);
    double minimum_initial_depth = std::numeric_limits<double>::infinity();
    Vector2F initial_normal = Vector2F::ZERO;
    const auto candidate_axes = axes(this->shape_);
    for (const Vector2F& axis : candidate_axes)
    {
        const Projection slab = project(this->shape_, axis), before = project(start, axis), after = project(finish, axis);
        const double negative_depth = before.maximum - slab.minimum;
        const double positive_depth = slab.maximum - before.minimum;
        if (negative_depth < minimum_initial_depth)
        { minimum_initial_depth = negative_depth; initial_normal = -axis; }
        if (positive_depth < minimum_initial_depth)
        { minimum_initial_depth = positive_depth; initial_normal = axis; }
        if (!clip(negative_depth, after.maximum - slab.minimum, -axis, result) ||
            !clip(positive_depth, slab.maximum - after.minimum, axis, result))
        { return std::nullopt; }
    }
    if (result.initially_overlapping || (result.entry == 0.0 && result.normal_to_rectangle == Vector2F::ZERO))
    { result.normal_to_rectangle = initial_normal; }

    // Every separating inequality is affine. If strict overlap exists anywhere
    // in the closed interval, it also exists at its interior midpoint. Testing
    // all four axes there rejects edge slides and vertex grazes, even when their
    // closed interval has a positive duration.
    if (result.entry < result.exit)
    {
        result.positive_overlap = true;
        const double midpoint = result.entry + (result.exit - result.entry) / 2.0;
        for (const Vector2F& axis : candidate_axes)
        {
            const Projection slab = project(this->shape_, axis), before = project(start, axis), after = project(finish, axis);
            const double minimum = before.minimum + (after.minimum - before.minimum) * midpoint;
            const double maximum = before.maximum + (after.maximum - before.maximum) * midpoint;
            if (maximum <= slab.minimum || minimum >= slab.maximum)
            { result.positive_overlap = false; break; }
        }
    }
    return result;
}
