#include "editor/level_json.h"
#include "game/level_format.h"
#include "editor/horizontal_authoring.h"
#include "editor/slab_authoring.h"
#include "game/station_contract.h"
#include "game/team_gate_contract.h"

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
#include <vector>

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
            if (string(value, "type") == "FixedSlabPaintable") { return decode_slab(value).bounds(); }
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

        namespace
        {
            // Which atlas look each material is, and the only collision tags it
            // may sit on: a jump-through is a ruler, a ramp a set square.
            struct Material
            {
                std::string_view name;
                std::string_view collision_types;
            };

            constexpr std::array<Material, 6> materials = { {
                { "ground", " STRUCTURE STRUCTURE_PAINTABLE " },
                { "pencil", " STRUCTURE_PAINTABLE " },
                { "eraser", " STRUCTURE_PAINTABLE " },
                { "paint_tin", " STRUCTURE_PAINTABLE " },
                { "ruler", " STRUCTURE_JUMP_THROUGH " },
                { "set_square", " STRUCTURE_RAMP_LEFT STRUCTURE_RAMP_RIGHT " } } };

            bool sits_on(const Material& material, std::string_view collision_type)
            {
                return !collision_type.empty() && collision_type.find(' ') == std::string_view::npos &&
                    material.collision_types.find(" " + std::string(collision_type) + " ") !=
                        std::string_view::npos;
            }
        }

        std::string allowed_materials(std::string_view collision_type)
        {
            std::string result;
            for (const Material& material : materials)
            {
                if (!sits_on(material, collision_type)) { continue; }
                if (!result.empty()) { result += ", "; }
                result += material.name;
            }
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

        // Optional. Absent keeps the legacy stretched sheet/frame draw.
        void validate_material(const Json& object, const std::string& collision_type)
        {
            if (!object.HasMember("material")) { return; }
            const std::string name = string(object, "material");
            const auto match = std::find_if(materials.begin(), materials.end(),
                [&](const Material& material) { return material.name == name; });
            if (match == materials.end())
            {
                fail("Unknown material '" + name +
                    "': use ground, pencil, eraser, paint_tin, ruler or set_square.");
            }
            if (!sits_on(*match, collision_type))
            {
                const std::string allowed = allowed_materials(collision_type);
                fail("Material '" + name + "' does not fit " + collision_type + " collision; " +
                    (allowed.empty() ? std::string("") : "use " + allowed + " or ") +
                    "remove the material.");
            }
        }

        void validate_drift(const Json& object, bool collision)
        {
            if (!object.HasMember("drift")) { return; }
            if (collision) { fail("Only decorative Visual entries may drift."); }
            const Json& drift = object["drift"];
            if (!drift.IsObject()) { fail("drift must be an object."); }
            const double distance = number(drift, "distance");
            const double period = number(drift, "period");
            const double phase = number(drift, "phase");
            if (distance <= 0.0 || distance > 256.0 || static_cast<float>(distance) <= 0.0f)
            {
                fail("drift.distance must be greater than zero and at most 256 world units.");
            }
            if (period < 1.0 || period > 3600.0)
            {
                fail("drift.period must be between 1 and 3600 seconds.");
            }
            if (phase < 0.0 || phase >= 1.0 || static_cast<float>(phase) >= 1.0f)
            {
                fail("drift.phase must be at least zero and less than one cycle.");
            }
        }

        // The startup check in game/content_validation.cpp: no spawn body may
        // overlap a gate, which could close on it.
        void validate_gate_spawns(const Document& document, const std::filesystem::path& tuning)
        {
            const Json& objects = array(document, "collision_objects");
            bool any = false;
            for (const Json& object : objects.GetArray()) { any = any || object.HasMember("gate"); }
            if (!any) { return; }
            const MotionPlayerSize player = load_motion_player_size(tuning);
            const Json& spawns = member(document, "spawn_locations");
            for (rapidjson::SizeType index = 0; index < objects.Size(); ++index)
            {
                const Json& object = objects[index];
                if (!object.HasMember("gate")) { continue; }
                const Rect box = object_rectangle(object);
                const team_gate_contract::Rectangle gate{ static_cast<float>(box.x), static_cast<float>(box.y),
                    static_cast<float>(box.width), static_cast<float>(box.height) };
                for (const char* team : { "team_a", "team_b" })
                {
                    for (const Json& point : array(spawns, team).GetArray())
                    {
                        try
                        {
                            team_gate_contract::require_clear_of_spawn(gate, { static_cast<float>(number(point, "x")),
                                static_cast<float>(number(point, "y")), player.width, player.height });
                        }
                        catch (const std::invalid_argument& error)
                        {
                            std::string name = "collision_objects[" + std::to_string(index) + "]";
                            if (object.HasMember("name") && object["name"].IsString())
                            { name += " ('" + string(object, "name") + "')"; }
                            fail(name + ": gate: " + error.what());
                        }
                    }
                }
            }
        }

        // The game's own station rules (LevelLoadedInfo::validate_stations), so
        // a bounds or station edit the game would refuse at load is refused
        // here, in its words. Lesson words are only checked for emptiness: the
        // game alone owns the fonts that decide what can be drawn.
        void validate_stations(const Document& document, const Rect& arena)
        {
            if (!document.HasMember("stations")) { return; }
            const Json& stations = array(document, "stations");
            const auto contract_rectangle = [](const Rect& rectangle)
                {
                    return station_contract::Rectangle{ static_cast<float>(rectangle.x),
                        static_cast<float>(rectangle.y), static_cast<float>(rectangle.width),
                        static_cast<float>(rectangle.height) };
                };
            const station_contract::Rectangle out_of_bounds = contract_rectangle(arena);
            const station_contract::Rectangle camera = contract_rectangle(
                plain_rectangle(member(member(document, "level_bounds"), "camera_bounds")));
            std::vector<std::string> seen;
            for (rapidjson::SizeType index = 0; index < stations.Size(); ++index)
            {
                const Json& station = stations[index];
                std::string where = "stations[" + std::to_string(index) + "]";
                if (!station.IsObject()) { fail(where + " must be a JSON object."); }
                const std::string name = string(station, "name");
                if (!name.empty()) { where += " ('" + name + "')"; }
                const station_contract::Point point{ static_cast<float>(number(station, "x")),
                    static_cast<float>(number(station, "y")) };
                try
                {
                    station_contract::validate_name(name, seen);
                    station_contract::validate_position(name, point, out_of_bounds);
                }
                catch (const std::invalid_argument& error) { fail(where + " " + error.what()); }
                if (!station.HasMember("lesson")) { continue; }
                const Json& lesson = member(station, "lesson");
                try
                {
                    // Read as numbers, not plain_rectangle, so a flat area gets the game's words.
                    const Json& area = member(lesson, "area");
                    station_contract::validate_lesson_area(name, point,
                        { static_cast<float>(number(area, "x")), static_cast<float>(number(area, "y")),
                            static_cast<float>(number(area, "width")), static_cast<float>(number(area, "height")) },
                        camera);
                }
                catch (const std::invalid_argument& error) { fail(where + ".lesson.area " + error.what()); }
                for (const char* key : { "title", "text" })
                {
                    try { station_contract::validate_lesson_words(string(lesson, key)); }
                    catch (const std::invalid_argument& error)
                    {
                        fail(where + ".lesson." + key + " " + error.what());
                    }
                }
            }
        }

        void validate_object(const Json& object, bool collision)
        {
            if (!object.IsObject())
            {
                fail("Every level object must be a JSON object.");
            }
            const std::string type = string(object, "type");
            if (object.HasMember("gate"))
            {
                // The game's own rule, so the editor refuses exactly the gates
                // the game refuses at load, in its words.
                team_gate_contract::Record record;
                if (!object["gate"].IsString()) { fail("gate: gate must be 'paint'"); }
                record.gate = string(object, "gate");
                record.structure_paintable = collision && type == "StructurePaintable";
                record.moving = object.HasMember("motion");
                if (record.structure_paintable)
                {
                    record.solid = string(object, "collision_type") == "STRUCTURE_PAINTABLE";
                    const Json& faces = member(object, "paintable_faces");
                    for (const char* face : { "left", "top", "right", "bottom" })
                    {
                        const Json& value = member(faces, face);
                        record.any_paintable_face = record.any_paintable_face || (value.IsBool() && value.GetBool());
                    }
                }
                try { team_gate_contract::decode(record); }
                catch (const std::invalid_argument& error) { fail(std::string("gate: ") + error.what()); }
            }
            if (type == "FixedSlabPaintable")
            {
                if (!collision) { fail("A fixed slab is collision geometry, not decoration."); }
                validate_slab(object);
                return;
            }
            if (object.HasMember("motion"))
            {
                try { decode_horizontal_motion(object); }
                catch (const std::exception& error) { fail(std::string("motion: ") + error.what()); }
            }
            if (string(object, "sheet_name").empty() || string(object, "frame_name").empty())
            {
                fail("Sprite sheet and frame names must not be empty.");
            }
            if (!known_colour(string(object, "colour")))
            {
                fail("Scenery is grey: use a grey colour name such as GAINSBORO, or RRGGBB with three equal pairs.");
            }
            validate_drift(object, collision);
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
                validate_material(object, collision_type);
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
                if (collision_type != "STRUCTURE_PAINTABLE" && collision_type != "STRUCTURE_JUMP_THROUGH")
                {
                    fail("A paintable structure needs STRUCTURE_PAINTABLE or STRUCTURE_JUMP_THROUGH collision.");
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
            validate_material(object, collision_type);
            check_rectangle(object_rectangle(object));
        }
    }

    namespace level_json
    {
        void validate_document(const Document& document, const std::filesystem::path& motion_tuning)
        {
            if (!document.IsObject()) { fail("A level must be a JSON object."); }
            // Missing format is legacy version 1. Reject newer or malformed
            // versions before interpreting any fields in their vocabulary.
            if (document.HasMember("format"))
            {
                const Json& format = document["format"];
                if (!format.IsInt())
                {
                    fail("format must be an integer; supported level format is " +
                        std::to_string(level_format::current) + ".");
                }
                if (format.GetInt() != level_format::current)
                {
                    fail("Unsupported level format " + std::to_string(format.GetInt()) +
                        "; supported format is " + std::to_string(level_format::current) + ".");
                }
            }
            validate_display_name(string(document, "display_name"));
            if (document.HasMember("include_in_random") && !document["include_in_random"].IsBool())
            {
                fail("include_in_random must be true or false.");
            }
            if (string(document, "icon_frame").empty()) { fail("The level needs an icon frame."); }
            const Json& sound = member(document, "sound");
            if (string(sound, "sound_bank_name").empty()) { fail("The level needs a sound bank."); }
            // Music is optional as a pair; a stage with neither is silent.
            const bool named = sound.HasMember("music_name");
            if (named != sound.HasMember("music_volume"))
            {
                fail("Music name and music volume come together, or neither for a silent stage.");
            }
            if (named && string(sound, "music_name").empty()) { fail("The music name must not be empty."); }
            if (named && (number(sound, "music_volume") < 0.0 || number(sound, "music_volume") > 1.0))
            {
                fail("music_volume must be 0..1.");
            }
            const bool sting = sound.HasMember("end_sting_name");
            if (sting != sound.HasMember("end_sting_volume"))
            {
                fail("end_sting_name and end_sting_volume come together, or neither.");
            }
            if (sting && string(sound, "end_sting_name").empty()) { fail("end_sting_name must not be empty."); }
            if (sting && (number(sound, "end_sting_volume") < 0.0 || number(sound, "end_sting_volume") > 1.0))
            {
                fail("end_sting_volume must be 0..1.");
            }
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
            validate_slab_pairs(document);
            validate_motion_geometry(document, motion_tuning);
            validate_gate_spawns(document, motion_tuning);
            validate_stations(document, arena);
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
            // RapidJSON also treats an actual zero inside bounded input as EOF.
            // Valid JSON represents NUL with an escape in the source text.
            const size_t nul = text.find('\0');
            if (nul != std::string_view::npos)
            {
                fail("Invalid JSON at byte " + std::to_string(nul) + ": Unescaped NUL byte.");
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
