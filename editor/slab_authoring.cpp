#include "editor/slab_authoring.h"
#include "game/fixed_slab_paint_geometry.h"

#include <algorithm>
#include <cmath>

namespace colourwars_editor
{
    using namespace level_json;

    PaintPoint SlabGeometry::to_world(float x, float y) const
    {
        return { (center_x + tangent_x * (x - length / 2.0f)) + normal_x * (y - 12.0f),
            (center_y + tangent_y * (x - length / 2.0f)) + normal_y * (y - 12.0f) };
    }

    PaintPoint SlabGeometry::to_local(const PaintPoint& point) const
    {
        const double dx = point.x - center_x, dy = point.y - center_y;
        const double norm = static_cast<double>(tangent_x) * tangent_x + static_cast<double>(tangent_y) * tangent_y;
        return { static_cast<float>((dx * tangent_x + dy * tangent_y) / norm + length / 2.0),
            static_cast<float>((dx * normal_x + dy * normal_y) / norm + 12.0) };
    }

    Rect SlabGeometry::bounds() const
    {
        double left = corners[0].x, right = left, top = corners[0].y, bottom = top;
        for (const PaintPoint& point : corners)
        {
            left = std::min(left, point.x); right = std::max(right, point.x);
            top = std::min(top, point.y); bottom = std::max(bottom, point.y);
        }
        // RectangleF's bounds store their size in float too.
        return { left, top, static_cast<float>(right - left), static_cast<float>(bottom - top) };
    }

    SlabGeometry decode_slab(const Json& object)
    {
        const Json& slab = member(object, "slab");
        if (!slab.IsObject()) { fail("slab must be an object."); }
        const float x = static_cast<float>(number(slab, "x")), y = static_cast<float>(number(slab, "y"));
        const std::string rise = string(slab, "rise");
        if (rise != "left" && rise != "right") { fail("slab.rise must be left or right."); }
        const float dy = rise == "right" ? -80.0f : 80.0f;
        const float end_x = x + 160.0f, end_y = y + dy;
        for (float value : { x, y, end_x, end_y })
            if (!std::isfinite(value) || std::abs(value) > 65536.0f)
                fail("Fixed slab endpoints must be finite and within 65536 world units.");
        if (end_x - x != 160.0f || end_y - y != dy)
            fail("Fixed slab endpoint precision cannot represent its fixed footprint.");
        SlabGeometry result;
        result.length = std::hypot(160.0f, dy);
        const float tx = 160.0f / result.length, ty = dy / result.length;
        const float nx = -ty, ny = tx;
        result.center_x = (x + 80.0f) + nx * 12.0f;
        result.center_y = (y + dy / 2.0f) + ny * 12.0f;
        // RectangleRotated normalizes both supplied axes before caching corners.
        const float tangent_length = std::sqrt(tx * tx + ty * ty);
        const float normal_length = std::sqrt(nx * nx + ny * ny);
        result.tangent_x = tx / tangent_length; result.tangent_y = ty / tangent_length;
        result.normal_x = nx / normal_length; result.normal_y = ny / normal_length;
        result.corners = { result.to_world(0, 0), result.to_world(result.length, 0),
            result.to_world(result.length, 24), result.to_world(0, 24) };
        result.paint_corners = canonical_slab_paint_polygon(result.corners,
            { x, y }, { end_x, end_y }, { result.normal_x, result.normal_y });
        return result;
    }

    void validate_slab(const Json& object)
    {
        if (level_format::collision_named(string(object, key::collision_type)) !=
                level_format::Collision::structure_fixed_slab)
            fail("A fixed slab needs STRUCTURE_FIXED_SLAB collision.");
        if (string(object, key::material) != level_format::fixed_slab_material)
            fail("A fixed slab uses the eraser material.");
        for (const char* field : level_format::fixed_slab_refused_keys)
            if (object.HasMember(field)) { fail(std::string("Fixed slab does not support '") + field + "'."); }
        const Json& slab = member(object, key::slab);
        if (!slab.IsObject()) { fail("slab must be an object."); }
        for (const char* field : level_format::fixed_slab_refused_slab_keys)
            if (slab.HasMember(field)) { fail(std::string("Fixed slab does not support slab.") + field + "."); }
        (void)decode_slab(object);
        const Json& faces = member(object, "paintable_faces");
        for (const char* face : { "left", "top", "right", "bottom" })
            if (!member(faces, face).IsBool()) { fail(std::string("Paintable face ") + face + " must be true or false."); }
    }

    void move_slab(Json& object, const Rect& rectangle)
    {
        const auto geometry = decode_slab(object);
        const Rect old = geometry.bounds();
        if (std::abs(rectangle.width - old.width) > 0.000001 || std::abs(rectangle.height - old.height) > 0.000001)
            fail("A fixed slab can move, but its footprint cannot be resized.");
        Json& slab = member(object, "slab");
        slab["x"].SetDouble(number(slab, "x") + rectangle.x - old.x);
        slab["y"].SetDouble(number(slab, "y") + rectangle.y - old.y);
    }

    void validate_slab_pairs(const Document& document)
    {
        std::vector<SlabGeometry> earlier;
        for (const Json& object : array(document, "collision_objects").GetArray())
        {
            if (string(object, key::type) != object_type::fixed_slab_paintable) { continue; }
            const SlabGeometry slab = decode_slab(object);
            for (const SlabGeometry& other : earlier)
            {
                // Match the pinned engine's RectangleRotated SAT: its stored
                // axes and float dot products are authoritative, rather than
                // normals reconstructed from rounded corners or an AABB.
                const std::array<PaintPoint, 4> axes{ PaintPoint{ slab.tangent_x, slab.tangent_y },
                    PaintPoint{ slab.normal_x, slab.normal_y }, PaintPoint{ other.tangent_x, other.tangent_y },
                    PaintPoint{ other.normal_x, other.normal_y } };
                bool overlaps = true;
                for (const PaintPoint& axis : axes)
                {
                    const auto project = [&](const PaintPolygon& polygon)
                    {
                        const auto dot = [&](const PaintPoint& point)
                        { return static_cast<float>(point.x) * static_cast<float>(axis.x) +
                            static_cast<float>(point.y) * static_cast<float>(axis.y); };
                        float low = dot(polygon[0]), high = low;
                        for (std::size_t index = 1; index < polygon.size(); ++index)
                        { const float distance = dot(polygon[index]); low = std::min(low, distance); high = std::max(high, distance); }
                        return std::pair{ low, high };
                    };
                    const auto a = project(slab.corners), b = project(other.corners);
                    if (!(b.second - a.first > 0.0f && a.second - b.first > 0.0f))
                    { overlaps = false; break; }
                }
                if (overlaps) { fail("Intersecting fixed slabs are unsupported."); }
            }
            earlier.push_back(slab);
        }
    }
}
