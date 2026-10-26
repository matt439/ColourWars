#include "editor/horizontal_authoring.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <string>

namespace colourwars_editor
{
    using namespace level_json;
    namespace motion = horizontal_motion_contract;

    namespace
    {
        motion::Rectangle plain_motion_rectangle(const Json& rectangle)
        {
            return { static_cast<float>(number(rectangle, "x")), static_cast<float>(number(rectangle, "y")),
                static_cast<float>(number(rectangle, "width")), static_cast<float>(number(rectangle, "height")) };
        }

        Rect rect(const motion::Rectangle& value)
        { return { value.x, value.y, value.width, value.height }; }

        Rect rect(const motion::Box& value)
        { return { value.left, value.top, value.right - value.left, value.bottom - value.top }; }

        std::string object_name(const Json& object, size_t index)
        {
            std::string result = "collision_objects[" + std::to_string(index) + "]";
            if (object.HasMember("name") && object["name"].IsString()) { result += " ('" + string(object, "name") + "')"; }
            return result;
        }
    }

    motion::Rectangle motion_rectangle(const Json& object)
    {
        if (string(object, "type") == "StructureRamp")
        {
            const Json& triangle = member(object, "triangle");
            const std::array<float, 3> xs{ static_cast<float>(number(triangle, "x1")),
                static_cast<float>(number(triangle, "x2")), static_cast<float>(number(triangle, "x3")) };
            const std::array<float, 3> ys{ static_cast<float>(number(triangle, "y1")),
                static_cast<float>(number(triangle, "y2")), static_cast<float>(number(triangle, "y3")) };
            const auto [left, right] = std::minmax_element(xs.begin(), xs.end());
            const auto [top, bottom] = std::minmax_element(ys.begin(), ys.end());
            return { *left, *top, *right - *left, *bottom - *top };
        }
        if (object.HasMember("rectangle")) { return plain_motion_rectangle(object["rectangle"]); }
        const Json& bounds = member(object, "rectangle_tlbr");
        const float left = static_cast<float>(number(bounds, "left"));
        const float top = static_cast<float>(number(bounds, "top"));
        return { left, top, static_cast<float>(number(bounds, "right")) - left,
            static_cast<float>(number(bounds, "bottom")) - top };
    }

    motion::Spec decode_horizontal_motion(const Json& object)
    {
        if (string(object, "type") != "StructurePaintable")
        { fail("motion belongs only on flat StructurePaintable entries."); }
        const Json& fields = member(object, "motion");
        if (string(fields, "type") != "horizontal") { fail("motion.type supports only horizontal."); }
        for (const char* field : { "axis", "travel_y", "rotation" })
        {
            if (fields.HasMember(field)) { fail(std::string("Unsupported motion field: ") + field); }
        }
        const motion::Spec spec{ motion_rectangle(object), static_cast<float>(number(fields, "travel_x")),
            static_cast<float>(number(fields, "period")), static_cast<float>(number(fields, "phase")) };
        try { motion::validate_spec(spec); }
        catch (const std::invalid_argument& error) { fail(std::string("motion: ") + error.what()); }
        return spec;
    }

    MotionPlayerSize load_motion_player_size(const std::filesystem::path& tuning)
    {
        if (tuning.empty()) { fail("motion needs an explicit source tuning.json authoring context for player size."); }
        std::ifstream input(tuning, std::ios::binary);
        if (!input) { fail("motion cannot read source tuning.json: " + tuning.string()); }
        const std::string text{ std::istreambuf_iterator<char>(input), {} };
        if (input.bad()) { fail("motion could not read complete source tuning.json: " + tuning.string()); }
        const Document document = parse(text);
        const Json& size = member(member(document, "player"), "size");
        const MotionPlayerSize result{ static_cast<float>(number(size, "x")), static_cast<float>(number(size, "y")) };
        try { motion::box({ 0.0f, 0.0f, result.width, result.height }); }
        catch (const std::invalid_argument& error) { fail(std::string("motion player.size: ") + error.what()); }
        return result;
    }

    MotionGuide horizontal_motion_guide(const Json& object, const MotionPlayerSize& player,
        const std::optional<Rect>& preview)
    {
        motion::Spec spec = decode_horizontal_motion(object);
        if (preview)
        {
            spec.anchor = { static_cast<float>(preview->x), static_cast<float>(preview->y),
                static_cast<float>(preview->width), static_cast<float>(preview->height) };
        }
        const auto path = motion::path(spec);
        const auto initial = motion::pose(spec, motion::offset_at(spec, spec.initial_phase));
        return { rect(spec.anchor), rect(initial), rect(path), rect(motion::standing(path, player.width, player.height)) };
    }

    void validate_motion_geometry(const Document& document, const std::filesystem::path& tuning)
    {
        const Json& objects = array(document, "collision_objects");
        bool moving = false;
        for (const Json& object : objects.GetArray()) { moving |= object.HasMember("motion"); }
        if (!moving) { return; }
        const MotionPlayerSize player = load_motion_player_size(tuning);
        const auto world = motion::box(plain_motion_rectangle(member(member(document, "level_bounds"), "out_of_bounds")));
        for (rapidjson::SizeType index = 0; index < objects.Size(); ++index)
        {
            const Json& object = objects[index];
            if (!object.HasMember("motion")) { continue; }
            try
            {
                const auto path = motion::path(decode_horizontal_motion(object));
                if (!motion::contains(world, path)) { fail("motion platform envelope leaves Scene bounds."); }
                const auto standing = motion::standing(path, player.width, player.height);
                if (!motion::contains(world, standing)) { fail("motion standing envelope leaves Scene bounds."); }
                for (rapidjson::SizeType other = 0; other < objects.Size(); ++other)
                {
                    if (other == index) { continue; }
                    const auto actual = motion_rectangle(objects[other]);
                    const auto obstacle = objects[other].HasMember("motion") ?
                        motion::path(decode_horizontal_motion(objects[other])) : motion::box(actual);
                    motion::finite(static_cast<double>(player.width) + actual.width);
                    motion::finite(static_cast<double>(player.height) + actual.height);
                    if (motion::intersects(path, obstacle))
                    { fail("motion platform envelope intersects " + object_name(objects[other], other) + "."); }
                    if (motion::intersects(standing, obstacle))
                    { fail("motion standing envelope intersects " + object_name(objects[other], other) + "."); }
                }
            }
            catch (const std::exception& error) { fail(object_name(object, index) + ": " + error.what()); }
        }
    }
}
