#include "editor/blockout_aids.h"
#include "editor/level_json.h"
#include "game/paint_face_geometry.h"
#include "game/fixed_slab_paint_geometry.h"
#include "editor/horizontal_authoring.h"
#include "editor/slab_authoring.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace colourwars_editor
{
    namespace
    {
        level_json::Document read_json(const std::filesystem::path& path)
        {
            std::ifstream stream(path, std::ios::binary);
            if (!stream) throw std::runtime_error("Cannot read editor aid data: " + path.string());
            const std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
            return level_json::parse(text);
        }

        PaintFaceRectangle paint_rectangle(const level_json::Json& object)
        {
            // Decode each number to float at the same point as the game,
            // including subtraction in the alternate TLBR encoding.
            using level_json::number;
            if (object.HasMember("rectangle"))
            {
                const level_json::Json& rect = level_json::member(object, "rectangle");
                const float x = static_cast<float>(number(rect, "x")), y = static_cast<float>(number(rect, "y"));
                return {x, y, x + static_cast<float>(number(rect, "width")), y + static_cast<float>(number(rect, "height"))};
            }
            const level_json::Json& rect = level_json::member(object, "rectangle_tlbr");
            const float x = static_cast<float>(number(rect, "left")), y = static_cast<float>(number(rect, "top"));
            const float width = static_cast<float>(number(rect, "right")) - x;
            const float height = static_cast<float>(number(rect, "bottom")) - y;
            return {x, y, x + width, y + height};
        }

        std::vector<std::pair<float, float>> slab_intervals(const SlabGeometry& slab,
            bool along_x, bool leading, std::span<const PaintFaceRectangle> obstacles)
        {
            return exposed_slab_paint_edge(slab.paint_corners, along_x ? slab.length : 24.0f,
                along_x, leading, obstacles);
        }

        double polygon_area(const std::vector<PaintPoint>& polygon)
        {
            double twice = 0;
            for (std::size_t i = 0; i < polygon.size(); ++i)
            {
                const auto& a = polygon[i]; const auto& b = polygon[(i + 1) % polygon.size()];
                twice += a.x * b.y - a.y * b.x;
            }
            return std::abs(twice) / 2.0;
        }

        double polygon_region_area(const PaintPolygon& polygon, const Rect& region)
        {
            std::vector<PaintPoint> points(polygon.begin(), polygon.end());
            for (int side = 0; side < 4 && !points.empty(); ++side)
            {
                const bool x_axis = side < 2, leading = side % 2 == 0;
                const double edge = x_axis ? (leading ? region.x : region.x + region.width)
                    : (leading ? region.y : region.y + region.height);
                const auto component = [x_axis](const PaintPoint& point) { return x_axis ? point.x : point.y; };
                const auto inside = [&](const PaintPoint& point)
                { return leading ? component(point) >= edge : component(point) <= edge; };
                std::vector<PaintPoint> next;
                for (std::size_t i = 0; i < points.size(); ++i)
                {
                    const auto& a = points[i]; const auto& b = points[(i + 1) % points.size()];
                    const bool ai = inside(a), bi = inside(b);
                    if (ai) { next.push_back(a); }
                    if (ai != bi)
                    {
                        const double fraction = (edge - component(a)) / (component(b) - component(a));
                        next.push_back({ a.x + (b.x - a.x) * fraction, a.y + (b.y - a.y) * fraction });
                    }
                }
                points = std::move(next);
            }
            return polygon_area(points);
        }
    }

    BlockoutTuning load_blockout_tuning(const std::filesystem::path& path,
        const std::filesystem::path& measurements)
    {
        const level_json::Document json = read_json(path);
        const level_json::Json& player = level_json::member(json, "player");
        const level_json::Json& size = level_json::member(player, "size");
        BlockoutTuning result{level_json::number(size, "x"), level_json::number(size, "y"),
            level_json::number(level_json::member(json, "paint"), "thickness"), true};
        for (double value : {result.player_width, result.player_height, result.paint_thickness})
            if (!std::isfinite(value) || value <= 0.0)
                throw std::runtime_error("Editor player size and paint thickness must be positive and finite.");
        const level_json::Document baseline = read_json(measurements);
        const level_json::Json& expected = level_json::member(baseline, "player");
        if (!expected.IsObject()) throw std::runtime_error("Movement baseline needs a player tuning snapshot");
        for (const level_json::Json::Member& field : expected.GetObject())
        {
            const char* name = field.name.GetString();
            if (field.value.IsObject())
            {
                const level_json::Json& actual = level_json::member(player, name);
                for (const level_json::Json::Member& component : field.value.GetObject())
                    result.measured_movement_matches &= level_json::number(actual, component.name.GetString())
                        == level_json::number(field.value, component.name.GetString());
            }
            else result.measured_movement_matches &= level_json::number(player, name) == level_json::number(expected, name);
        }
        const level_json::Json& tap = level_json::member(baseline, "tap");
        const level_json::Json& held = level_json::member(baseline, "held");
        result.tap_rise = level_json::number(tap, "rise");
        result.tap_travel = level_json::number(tap, "travel");
        result.held_rise = level_json::number(held, "rise");
        result.held_travel = level_json::number(held, "travel");
        result.measured_speed = level_json::number(baseline, "speed");
        for (double value : {result.tap_rise, result.tap_travel, result.held_rise,
            result.held_travel, result.measured_speed, level_json::number(baseline, "timestep")})
            if (value <= 0.0) throw std::runtime_error("Movement measurements must be positive");
        return result;
    }

    std::vector<ScoringFace> scoring_faces(const LevelDocument& document, const BlockoutTuning& tuning)
    {
        const level_json::Document json = level_json::parse(document.serialize());
        const level_json::Json& objects = level_json::array(json, "collision_objects");
        std::vector<PaintFaceRectangle> obstacles;
        std::vector<PaintPolygon> slabs;
        for (const level_json::Json& object : objects.GetArray())
        {
            const std::string type = level_json::string(object, "type");
            if (type == "FixedSlabPaintable") { slabs.push_back(decode_slab(object).paint_corners); continue; }
            if (type != "Structure" && type != "StructurePaintable") continue;
            if (object.HasMember("motion")) continue;
            obstacles.push_back(paint_rectangle(object));
        }
        std::vector<ScoringFace> result;
        for (const Entry& entry : document.entries())
        {
            if (entry.kind != EntryKind::collision) continue;
            const level_json::Json& object = objects[static_cast<rapidjson::SizeType>(entry.index)];
            if (level_json::string(object, "type") == "FixedSlabPaintable")
            {
                const auto slab = decode_slab(object);
                const auto& faces = level_json::member(object, "paintable_faces");
                const float thickness = static_cast<float>(tuning.paint_thickness);
                if (!(thickness > 0.0f && thickness <= 12.0f))
                    throw std::runtime_error("Fixed slab paint thickness must fit its normal thickness.");
                for (int face = 0; face < 4; ++face)
                {
                    const char* names[] = { "top", "bottom", "left", "right" };
                    if (!level_json::member(faces, names[face]).GetBool()) { continue; }
                    const bool along_x = face < 2, leading = face % 2 == 0;
                    const float edge = along_x ? (leading ? 0.0f : 24.0f) : (leading ? 0.0f : slab.length);
                    for (const auto& [begin, end] : slab_intervals(slab, along_x, leading, obstacles))
                    {
                        const float x = along_x ? begin : (leading ? edge : edge - thickness);
                        const float y = along_x ? (leading ? edge : edge - thickness) : begin;
                        const float width = along_x ? end - begin : thickness;
                        const float height = along_x ? thickness : end - begin;
                        const PaintPolygon polygon{ slab.to_world(x, y), slab.to_world(x + width, y),
                            slab.to_world(x + width, y + height), slab.to_world(x, y + height) };
                        result.push_back({ entry, Rect{ x, y, width, height }, along_x, leading, polygon });
                    }
                }
                continue;
            }
            if (level_json::string(object, "type") != "StructurePaintable") continue;
            const PaintFaceRectangle rect = paint_rectangle(object);
            const bool moving = object.HasMember("motion");
            float offset = 0.0f;
            if (moving)
            {
                const auto spec = decode_horizontal_motion(object);
                offset = horizontal_motion_contract::offset_at(spec, spec.initial_phase);
            }
            const float x = rect.left, y = rect.top, right = rect.right, bottom = rect.bottom;
            const level_json::Json& faces = level_json::member(object, "paintable_faces");
            for (int face = 0; face < 4; ++face)
            {
                const char* names[] = {"top", "bottom", "left", "right"};
                if (!level_json::member(faces, names[face]).GetBool()) continue;
                const bool along_x = face < 2, leading = face % 2 == 0;
                const float edge = along_x ? (leading ? y : bottom) : (leading ? x : right);
                const float thickness = static_cast<float>(tuning.paint_thickness);
                const std::span<const PaintFaceRectangle> blockers = moving ?
                    std::span<const PaintFaceRectangle>{} : std::span<const PaintFaceRectangle>(obstacles);
                const auto exposed = exposed_paint_intervals(
                    along_x ? x : y, along_x ? right : bottom, edge, along_x, leading, blockers);
                const auto clipped = moving || slabs.empty() ? exposed : clip_paint_polygons(exposed,
                    along_x ? PaintPoint{ 0, edge } : PaintPoint{ edge, 0 },
                    along_x ? PaintPoint{ 1, 0 } : PaintPoint{ 0, 1 }, slabs);
                for (const std::pair<float, float>& interval : clipped)
                {
                    Rect strip = along_x
                        ? Rect{interval.first, leading ? edge : edge - thickness, interval.second - interval.first, thickness}
                        : Rect{leading ? edge : edge - thickness, interval.first, thickness, interval.second - interval.first};
                    // Runtime owns anchor-space scored areas. Translate only
                    // the displayed location: deriving a new width from world
                    // endpoints changes score when float spacing changes.
                    if (moving && offset != 0.0f) { strip.x = static_cast<float>(strip.x) + offset; }
                    result.push_back({entry, strip, along_x, leading});
                }
            }
        }
        return result;
    }

    double scoring_area(const std::vector<ScoringFace>& faces, const std::optional<Rect>& region)
    {
        double result = 0.0;
        for (const ScoringFace& face : faces)
        {
            const Rect& strip = face.strip;
            double width = strip.width, height = strip.height;
            if (face.polygon && region)
            {
                const std::vector<PaintPoint> whole(face.polygon->begin(), face.polygon->end());
                const double physical = polygon_area(whole);
                if (physical > 0) { result += width * height * polygon_region_area(*face.polygon, *region) / physical; }
                continue;
            }
            if (region)
            {
                width = std::max(0.0, std::min(strip.x + width, region->x + region->width) - std::max(strip.x, region->x));
                height = std::max(0.0, std::min(strip.y + height, region->y + region->height) - std::max(strip.y, region->y));
            }
            result += width * height;
        }
        return result;
    }

    SideScoring side_scoring_areas(const std::vector<ScoringFace>& faces, const Rect& out_of_bounds)
    {
        // Each half reaches well past the arena vertically and outwards, so
        // only the centre line divides a strip.
        constexpr double margin = 1.0e6;
        const double centre = out_of_bounds.x + out_of_bounds.width / 2.0;
        const double top = out_of_bounds.y - margin, height = out_of_bounds.height + 2.0 * margin;
        const double left_edge = out_of_bounds.x - margin;
        const double right_edge = out_of_bounds.x + out_of_bounds.width + margin;
        return { scoring_area(faces, Rect{ left_edge, top, centre - left_edge, height }),
            scoring_area(faces, Rect{ centre, top, right_edge - centre, height }) };
    }

    std::string symmetry_summary(const LevelDocument& document, const std::vector<ScoringFace>& faces)
    {
        const SideScoring sides = side_scoring_areas(faces, document.rectangle(Entry{ EntryKind::bounds, 0, "" }));
        const LevelDocument::SymmetryReport report = document.symmetry();
        const auto area = [](double value)
        {
            char text[32];
            std::snprintf(text, sizeof text, "%.1f", value);
            return std::string(text);
        };
        // A few names fit the canvas line; the count says how many there are.
        const auto names = [](const std::vector<Entry>& entries)
        {
            std::string text;
            for (std::size_t i = 0; i < entries.size() && i < 4; ++i) { text += (i ? ", " : "") + entries[i].label; }
            if (entries.size() > 4) { text += ", +" + std::to_string(entries.size() - 4) + " more"; }
            return text;
        };
        std::string text = "Symmetry: left " + area(sides.left) + " / right " + area(sides.right) + "  |  ";
        text += report.unmatched.empty() ? std::string("every object has a mirror partner")
            : std::to_string(report.unmatched.size()) + " without a mirror partner: " + names(report.unmatched);
        if (!report.unmatched_art.empty())
            text += "  |  Art without partner: " + names(report.unmatched_art);
        if (!report.paint_ferries.empty())
            text += "  |  Authored: paint-ferry heading (" + names(report.paint_ferries) + ")";
        return text;
    }
}
