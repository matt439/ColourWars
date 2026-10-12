#include "editor/level_json.h"

#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace colourwars_editor
{
    namespace level_json
    {
        [[noreturn]] void fail(const std::string& text)
        {
            throw std::runtime_error(text);
        }

        const Json& member(const Json& object, const char* name)
        {
            if (!object.IsObject() || !object.HasMember(name))
            {
                fail(std::string("Missing required field: ") + name);
            }
            return object[name];
        }

        Json& member(Json& object, const char* name)
        {
            return const_cast<Json&>(member(static_cast<const Json&>(object), name));
        }

        double number(const Json& object, const char* name)
        {
            const Json& value = member(object, name);
            if (!value.IsNumber() || !std::isfinite(value.GetDouble()) ||
                std::abs(value.GetDouble()) > static_cast<double>(std::numeric_limits<float>::max()))
            {
                fail(std::string(name) + " must be a finite game coordinate or number.");
            }
            return value.GetDouble();
        }

        std::string string(const Json& object, const char* name)
        {
            const Json& value = member(object, name);
            if (!value.IsString())
            {
                fail(std::string(name) + " must be text.");
            }
            return std::string(value.GetString(), value.GetStringLength());
        }

        const Json& array(const Json& object, const char* name)
        {
            const Json& value = member(object, name);
            if (!value.IsArray())
            {
                fail(std::string(name) + " must be an array.");
            }
            return value;
        }

        void check_rectangle(const Rect& rectangle)
        {
            for (double value : { rectangle.x, rectangle.y, rectangle.width, rectangle.height,
                rectangle.x + rectangle.width, rectangle.y + rectangle.height })
            {
                if (!std::isfinite(value) || std::abs(value) >
                    static_cast<double>(std::numeric_limits<float>::max()))
                {
                    fail("Rectangle coordinates must be finite game numbers.");
                }
            }
            if (rectangle.width <= 0.0 || rectangle.height <= 0.0 ||
                static_cast<float>(rectangle.width) <= 0.0f ||
                static_cast<float>(rectangle.height) <= 0.0f)
            {
                fail("Rectangle width and height must be positive.");
            }
        }

        Rect plain_rectangle(const Json& value)
        {
            const Rect result{ number(value, "x"), number(value, "y"),
                number(value, "width"), number(value, "height") };
            check_rectangle(result);
            return result;
        }

        Rect object_rectangle(const Json& value)
        {
            if (string(value, "type") == "StructureRamp")
            {
                const Json& triangle = member(value, "triangle");
                const std::array<double, 3> xs{ number(triangle, "x1"),
                    number(triangle, "x2"), number(triangle, "x3") };
                const std::array<double, 3> ys{ number(triangle, "y1"),
                    number(triangle, "y2"), number(triangle, "y3") };
                const auto [left, right] = std::minmax_element(xs.begin(), xs.end());
                const auto [top, bottom] = std::minmax_element(ys.begin(), ys.end());
                return { *left, *top, *right - *left, *bottom - *top };
            }
            if (value.HasMember("rectangle"))
            {
                return plain_rectangle(value["rectangle"]);
            }
            const Json& rectangle = member(value, "rectangle_tlbr");
            const double left = number(rectangle, "left");
            const double top = number(rectangle, "top");
            const Rect result{ left, top, number(rectangle, "right") - left,
                number(rectangle, "bottom") - top };
            check_rectangle(result);
            return result;
        }
    }

    namespace
    {
        using namespace level_json;

        bool known_colour(const std::string& colour)
        {
            // These are the CSS names the game's content loader accepts, including
            // its uppercase/underscore spelling, and only the grey ones: the game
            // refuses a hue in the scenery. RRGGBB greys are accepted below.
            constexpr std::string_view names =
                " BLACK DARK_GRAY DARK_GREY DIM_GRAY DIM_GREY GAINSBORO GRAY GREY LIGHT_GRAY LIGHT_GREY SILVER WHITE WHITE_SMOKE ";
            if (colour.length() == 6 &&
                colour.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos)
            {
                const auto pair = [&](size_t at) { return std::stoi(colour.substr(at, 2), nullptr, 16); };
                return pair(0) == pair(2) && pair(2) == pair(4);
            }
            return !colour.empty() && colour.find(' ') == std::string::npos &&
                names.find(" " + colour + " ") != std::string_view::npos;
        }

        void validate_display_name(std::string_view name)
        {
            // The shipped fonts cover printable ASCII. The game normalizes these
            // specific punctuation/space characters before requesting font glyphs.
            constexpr std::array<std::string_view, 18> punctuation = {
                "\xE2\x80\x98", "\xE2\x80\x99", "\xCA\xBC", "\xE2\x80\xB2",
                "\xE2\x80\x9C", "\xE2\x80\x9D", "\xE2\x80\xB3", "\xE2\x80\x90",
                "\xE2\x80\x91", "\xE2\x80\x93", "\xE2\x80\x94", "\xE2\x88\x92",
                "\xE2\x80\xA6", "\xC2\xA0", "\xE2\x80\x87", "\xE2\x80\xAF",
                "\n", "\r" };
            if (name.empty()) { fail("The level needs a display name."); }
            while (!name.empty())
            {
                if (name.front() >= ' ' && name.front() <= '~') { name.remove_prefix(1); continue; }
                const auto match = std::find_if(punctuation.begin(), punctuation.end(),
                    [name](std::string_view candidate) { return name.starts_with(candidate); });
                if (match == punctuation.end())
                {
                    fail("The stage name contains a character the game's fonts cannot draw; use printable ASCII letters.");
                }
                name.remove_prefix(match->size());
            }
        }

        void validate_object(const Json& object, bool collision)
        {
            if (!object.IsObject())
            {
                fail("Every level object must be a JSON object.");
            }
            const std::string type = string(object, "type");
            if (string(object, "sheet_name").empty() || string(object, "frame_name").empty())
            {
                fail("Sprite sheet and frame names must not be empty.");
            }
            if (!known_colour(string(object, "colour")))
            {
                fail("Scenery is grey: use a grey colour name such as GAINSBORO, or RRGGBB with three equal pairs.");
            }
            if (!collision)
            {
                if (type != "Visual")
                {
                    fail("Decoration entries must use type Visual.");
                }
                check_rectangle(object_rectangle(object));
                return;
            }
            const std::string collision_type = string(object, "collision_type");
            if (type == "StructureRamp")
            {
                if (collision_type != "STRUCTURE_RAMP_RIGHT" && collision_type != "STRUCTURE_RAMP_LEFT")
                {
                    fail("A ramp needs STRUCTURE_RAMP_RIGHT or STRUCTURE_RAMP_LEFT collision.");
                }
                const Rect box = object_rectangle(object);
                check_rectangle(box);
                const Json& triangle = member(object, "triangle");
                const bool right = collision_type == "STRUCTURE_RAMP_RIGHT";
                std::array<bool, 3> corners{};
                for (int index = 1; index <= 3; ++index)
                {
                    const double x = number(triangle, ("x" + std::to_string(index)).c_str());
                    const double y = number(triangle, ("y" + std::to_string(index)).c_str());
                    if (x == box.x && y == box.y + box.height) { corners[0] = true; }
                    else if (x == box.x + box.width && y == box.y + box.height) { corners[1] = true; }
                    else if (x == (right ? box.x + box.width : box.x) && y == box.y) { corners[2] = true; }
                    else { fail("Ramp points must form an upright axis-aligned ramp matching its direction."); }
                }
                if (!corners[0] || !corners[1] || !corners[2])
                {
                    fail("A ramp must have three distinct corners.");
                }
                return;
            }
            if (type == "Structure")
            {
                if (collision_type != "STRUCTURE" && collision_type != "STRUCTURE_JUMP_THROUGH")
                {
                    fail("A Structure needs STRUCTURE or STRUCTURE_JUMP_THROUGH collision.");
                }
            }
            else if (type == "StructurePaintable")
            {
                if (collision_type != "STRUCTURE_PAINTABLE")
                {
                    fail("A paintable structure needs STRUCTURE_PAINTABLE collision.");
                }
                const Json& faces = member(object, "paintable_faces");
                for (const char* face : { "left", "top", "right", "bottom" })
                {
                    if (!member(faces, face).IsBool())
                    {
                        fail(std::string("Paintable face ") + face + " must be true or false.");
                    }
                }
            }
            else { fail("Unknown collision object type: " + type); }
            check_rectangle(object_rectangle(object));
        }
    }

    namespace level_json
    {
        void validate_document(const Document& document)
        {
            if (!document.IsObject()) { fail("A level must be a JSON object."); }
            validate_display_name(string(document, "display_name"));
            if (string(document, "icon_frame").empty()) { fail("The level needs an icon frame."); }
            const Json& sound = member(document, "sound");
            if (string(sound, "sound_bank_name").empty() || string(sound, "music_name").empty())
            {
                fail("The level needs a sound bank and music name.");
            }
            if (number(sound, "music_volume") < 0.0) { fail("Music volume must not be negative."); }
            const Json& bounds = member(document, "level_bounds");
            for (const char* name : bounds_names) { plain_rectangle(member(bounds, name)); }
            const Rect arena = plain_rectangle(member(bounds, "out_of_bounds"));
            const Json& spawns = member(document, "spawn_locations");
            for (const char* team : { "team_a", "team_b" })
            {
                const Json& points = array(spawns, team);
                if (points.Size() < 4) { fail(std::string(team) + " needs at least four spawn points."); }
                for (const Json& point : points.GetArray())
                {
                    const double x = number(point, "x");
                    const double y = number(point, "y");
                    if (x < arena.x || x > arena.x + arena.width ||
                        y < arena.y || y > arena.y + arena.height)
                    {
                        fail(std::string(team) + " has a spawn outside the out-of-bounds rectangle.");
                    }
                    // The opening previews each team's colour over its own half.
                    const double middle = arena.x + arena.width / 2.0;
                    if ((std::string(team) == "team_a") != (x < middle))
                    {
                        fail("Team A spawns on the left half of the stage and team B on the right.");
                    }
                }
            }
            for (const Json& object : array(document, "collision_objects").GetArray())
            {
                validate_object(object, true);
            }
            for (const Json& object : array(document, "non_collision_objects").GetArray())
            {
                validate_object(object, false);
            }
        }

        Document parse(std::string_view text)
        {
            if (text.starts_with("\xEF\xBB\xBF")) { text.remove_prefix(3); }
            Document document;
            document.Parse<rapidjson::kParseValidateEncodingFlag>(text.data(), text.size());
            if (document.HasParseError())
            {
                fail("Invalid JSON at byte " + std::to_string(document.GetErrorOffset()) + ": " +
                    rapidjson::GetParseError_En(document.GetParseError()));
            }
            return document;
        }

        std::string json_text(const Json& value)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            writer.SetIndent(' ', 4);
            if (!value.Accept(writer)) { fail("The document cannot be serialized as JSON."); }
            return std::string(buffer.GetString(), buffer.GetSize()) + "\n";
        }
    }
}
