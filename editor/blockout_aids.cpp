#include "editor/blockout_aids.h"
#include "editor/level_json.h"
#include "game/paint_face_geometry.h"

#include <algorithm>
#include <cmath>
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
        for (const level_json::Json& object : objects.GetArray())
        {
            const std::string type = level_json::string(object, "type");
            if (type != "Structure" && type != "StructurePaintable") continue;
            obstacles.push_back(paint_rectangle(object));
        }
        std::vector<ScoringFace> result;
        for (const Entry& entry : document.entries())
        {
            if (entry.kind != EntryKind::collision) continue;
            const level_json::Json& object = objects[static_cast<rapidjson::SizeType>(entry.index)];
            if (level_json::string(object, "type") != "StructurePaintable") continue;
            const PaintFaceRectangle rect = paint_rectangle(object);
            const float x = rect.left, y = rect.top, right = rect.right, bottom = rect.bottom;
            const level_json::Json& faces = level_json::member(object, "paintable_faces");
            for (int face = 0; face < 4; ++face)
            {
                const char* names[] = {"top", "bottom", "left", "right"};
                if (!level_json::member(faces, names[face]).GetBool()) continue;
                const bool along_x = face < 2, leading = face % 2 == 0;
                const float edge = along_x ? (leading ? y : bottom) : (leading ? x : right);
                const float thickness = static_cast<float>(tuning.paint_thickness);
                for (const std::pair<float, float>& interval : exposed_paint_intervals(
                    along_x ? x : y, along_x ? right : bottom, edge, along_x, leading, obstacles))
                {
                    const Rect strip = along_x
                        ? Rect{interval.first, leading ? edge : edge - thickness, interval.second - interval.first, thickness}
                        : Rect{leading ? edge : edge - thickness, interval.first, thickness, interval.second - interval.first};
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
            if (region)
            {
                width = std::max(0.0, std::min(strip.x + width, region->x + region->width) - std::max(strip.x, region->x));
                height = std::max(0.0, std::min(strip.y + height, region->y + region->height) - std::max(strip.y, region->y));
            }
            result += width * height;
        }
        return result;
    }
}
