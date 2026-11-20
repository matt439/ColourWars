#pragma once

#include "game/contracts/fixed_slab_paint_geometry.h"
#include "game/contracts/level_format.h"
#include "game/contracts/paint_fill_region.h"
#include <rapidjson/document.h>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

// The game and standalone editor parse the same authored snapshot. Resolve
// names and face references once against its static scoring geometry.
namespace paint_fill_json
{
    namespace key = level_format::key;
    using Json = rapidjson::Value;

    [[noreturn]] inline void fail(const std::string& why)
    {
        throw std::invalid_argument("paint_fill: " + why);
    }

    inline const Json& member(const Json& object, const char* name)
    {
        if (!object.IsObject() || !object.HasMember(name)) { fail(std::string("missing '") + name + "'"); }
        return object[name];
    }

    // Unknown metadata is preserved. Each field defining the binding has one
    // meaning, even in a DOM which retains duplicate JSON member names.
    inline const Json& binding_member(const Json& object, const char* name)
    {
        const Json& value = member(object, name);
        const std::string_view key_name(name);
        for (const auto& item : object.GetObject())
        {
            if (&item.value != &value && std::string_view(item.name.GetString(), item.name.GetStringLength()) == key_name)
                fail("duplicate field '" + std::string(name) + "'");
        }
        return value;
    }

    inline std::string_view text(const Json& value)
    {
        if (!value.IsString() || !value.GetStringLength()) { fail("expected a non-empty string"); }
        return { value.GetString(), value.GetStringLength() };
    }

    inline float number(const Json& object, const char* name)
    {
        const Json& value = member(object, name);
        if (!value.IsNumber() || !(std::abs(value.GetDouble()) <= static_cast<double>((std::numeric_limits<float>::max)())))
            fail(std::string("invalid coordinate '") + name + "'");
        return static_cast<float>(value.GetDouble());
    }

    inline PaintFaceRectangle rectangle(const Json& object)
    {
        const bool xywh = object.HasMember(key::rectangle);
        const Json& rect = member(object, xywh ? key::rectangle : key::rectangle_tlbr);
        const float x = number(rect, xywh ? "x" : "left"), y = number(rect, xywh ? "y" : "top");
        const float w = xywh ? number(rect, "width") : number(rect, "right") - x;
        const float h = xywh ? number(rect, "height") : number(rect, "bottom") - y;
        if (!(w > 0 && h > 0)) { fail("expected a positive scoring rectangle"); }
        try { level_format::limit::check_object_rectangle(x, y, w, h); }
        catch (const std::invalid_argument& error)
        {
            fail(error.what());
        }
        return { x, y, x + w, y + h };
    }

    inline std::size_t resolve(const Json& objects, const Json& endpoint)
    {
        const auto name = text(binding_member(endpoint, "object"));
        std::optional<std::size_t> match;
        for (rapidjson::SizeType i = 0; i < objects.Size(); ++i)
        {
            const Json& object = objects[i];
            if (!object.IsObject() || !object.HasMember(key::name)) { continue; }
            const Json& value = object[key::name];
            if (!value.IsString() || std::string_view(value.GetString(), value.GetStringLength()) != name) { continue; }
            if (match) { fail("ambiguous object '" + std::string(name) + "'"); }
            match = i;
        }
        if (!match) { fail("unknown object '" + std::string(name) + "'"); }
        const Json& object = objects[static_cast<rapidjson::SizeType>(*match)];
        if (text(member(object, key::type)) != level_format::object_type::structure_paintable || object.HasMember(key::motion))
            fail("object '" + std::string(name) + "' must be a fixed StructurePaintable");
        return *match;
    }

    inline std::optional<PaintFillRegion> decode(const Json& root)
    {
        if (!root.IsObject() || !root.HasMember(key::paint_fill)) { return std::nullopt; }
        const Json& declaration = binding_member(root, key::paint_fill);
        const Json& trigger = binding_member(declaration, "trigger");
        const Json& target = binding_member(declaration, "target");
        const Json& objects = member(root, key::collision_objects);
        if (!objects.IsArray()) { fail("collision_objects must be an array"); }
        level_format::limit::check_object_count(objects.Size());
        PaintFillRegion region{ resolve(objects, trigger), {}, resolve(objects, target), PaintSurfaceFace::top };
        std::vector<PaintFaceRectangle> flat;
        std::vector<PaintPolygon> slabs;
        for (const Json& object : objects.GetArray())
        {
            const auto type = text(member(object, key::type));
            if (object.HasMember(key::motion)) { continue; }
            if (type == level_format::object_type::structure || type == level_format::object_type::structure_paintable)
                flat.push_back(rectangle(object));
            else if (type == level_format::object_type::fixed_slab_paintable)
            {
                const Json& slab = member(object, key::slab);
                const auto rise = text(member(slab, "rise"));
                if (rise != "left" && rise != "right") { fail("invalid slab rise"); }
                slabs.push_back(authored_slab_paint_polygon(number(slab, "x"), number(slab, "y"), rise == "right"));
            }
        }
        struct ScoringFace
        {
            PaintSurfaceFace side;
            float edge;
            std::vector<std::pair<float, float>> intervals;
        };
        const auto face = [&](std::size_t index, const Json& field) -> ScoringFace
        {
            const auto name = text(field);
            const Json& object = objects[static_cast<rapidjson::SizeType>(index)];
            const bool along_x = name == "top" || name == "bottom";
            const bool leading = name == "top" || name == "left";
            if (!along_x && name != "left" && name != "right") { fail("unknown face '" + std::string(name) + "'"); }
            const Json& enabled = member(member(object, key::paintable_faces), field.GetString());
            if (!enabled.IsBool() || !enabled.GetBool()) { fail("disabled face '" + std::string(name) + "'"); }
            const PaintFaceRectangle rect = rectangle(object);
            const float edge = along_x ? (leading ? rect.top : rect.bottom) : (leading ? rect.left : rect.right);
            const auto intervals = exposed_paint_intervals(along_x ? rect.left : rect.top, along_x ? rect.right : rect.bottom,
                edge, along_x, leading, flat);
            auto exposed = clip_paint_polygons(intervals, along_x ? PaintPoint{ 0, edge } : PaintPoint{ edge, 0 },
                along_x ? PaintPoint{ 1, 0 } : PaintPoint{ 0, 1 }, slabs);
            if (exposed.empty()) { fail("buried or empty face '" + std::string(name) + "'"); }
            const auto side = along_x ? (leading ? PaintSurfaceFace::top : PaintSurfaceFace::bottom)
                : (leading ? PaintSurfaceFace::left : PaintSurfaceFace::right);
            return { side, edge, std::move(exposed) };
        };
        const ScoringFace destination = face(region.target_object, binding_member(target, "face"));
        region.target_face = destination.side;
        const Json& faces = binding_member(trigger, "faces");
        if (!faces.IsArray() || faces.Empty() || faces.Size() > 4) { fail("trigger.faces needs one to four distinct faces"); }
        for (const Json& value : faces.GetArray())
        {
            const ScoringFace source = face(region.trigger_object, value);
            if (std::find(region.trigger_faces.begin(), region.trigger_faces.end(), source.side) != region.trigger_faces.end())
                fail("duplicate trigger face");
            // Even distinct records can present the same strip. Only positive
            // exposed length on the same oriented edge is shared paint.
            if (source.side == destination.side && source.edge == destination.edge)
            {
                auto target_strip = destination.intervals.begin();
                for (const auto& strip : source.intervals)
                {
                    while (target_strip != destination.intervals.end() && target_strip->second <= strip.first) { ++target_strip; }
                    if (target_strip == destination.intervals.end()) { break; }
                    if (target_strip->first < strip.second) { fail("trigger and target faces overlap"); }
                }
            }
            region.trigger_faces.push_back(source.side);
        }
        return region;
    }
}
