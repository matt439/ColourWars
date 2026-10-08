#include "editor/level_document.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/pointer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

namespace colourwars_editor
{
    namespace
    {
        using Json = rapidjson::Value;
        using Document = rapidjson::Document;
        using Allocator = Document::AllocatorType;

        constexpr std::array<const char*, 4> bounds_names = {
            "out_of_bounds", "camera_bounds", "zoom_out_start_bounds", "zoom_out_finish_bounds" };
        constexpr std::array<const char*, 4> bounds_labels = {
            "Out of bounds", "Camera bounds", "Zoom start", "Zoom finish" };

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

        Json& entry_json(Document& document, const Entry& entry)
        {
            switch (entry.kind)
            {
            case EntryKind::level:
                return document;
            case EntryKind::bounds:
                if (entry.index >= bounds_names.size())
                {
                    fail("The selected bounds no longer exist.");
                }
                return member(member(document, "level_bounds"), bounds_names[entry.index]);
            default:
                break;
            }
            Json* values = nullptr;
            if (entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b)
            {
                values = &member(member(document, "spawn_locations"),
                    entry.kind == EntryKind::spawn_a ? "team_a" : "team_b");
            }
            else
            {
                values = &member(document,
                    entry.kind == EntryKind::collision ? "collision_objects" : "non_collision_objects");
            }
            if (!values->IsArray() || entry.index >= values->Size())
            {
                fail("The selected entry no longer exists.");
            }
            return (*values)[static_cast<rapidjson::SizeType>(entry.index)];
        }

        Json& entry_array(Document& document, EntryKind kind)
        {
            if (kind == EntryKind::spawn_a || kind == EntryKind::spawn_b)
            {
                return member(member(document, "spawn_locations"),
                    kind == EntryKind::spawn_a ? "team_a" : "team_b");
            }
            if (kind == EntryKind::collision || kind == EntryKind::decoration)
            {
                return member(document,
                    kind == EntryKind::collision ? "collision_objects" : "non_collision_objects");
            }
            fail("Level metadata and bounds cannot be duplicated or deleted.");
        }

        bool known_colour(const std::string& colour)
        {
            // These are the CSS names the game's content loader accepts, including
            // its uppercase/underscore spelling. No hexadecimal colour extension.
            constexpr std::string_view names =
                " ALICE_BLUE ANTIQUE_WHITE AQUA AQUAMARINE AZURE BEIGE BISQUE BLACK BLANCHED_ALMOND"
                " BLUE BLUE_VIOLET BROWN BURLYWOOD CADET_BLUE CHARTREUSE CHOCOLATE CORAL CORNFLOWER_BLUE"
                " CORNSILK CRIMSON CYAN DARK_BLUE DARK_CYAN DARK_GOLDENROD DARK_GRAY DARK_GREEN DARK_GREY"
                " DARK_KHAKI DARK_MAGENTA DARK_OLIVE_GREEN DARK_ORANGE DARK_ORCHID DARK_RED DARK_SALMON"
                " DARK_SEA_GREEN DARK_SLATE_BLUE DARK_SLATE_GRAY DARK_SLATE_GREY DARK_TURQUOISE DARK_VIOLET"
                " DEEP_PINK DEEP_SKY_BLUE DIM_GRAY DIM_GREY DODGER_BLUE FIRE_BRICK FLORAL_WHITE FOREST_GREEN"
                " FUCHSIA GAINSBORO GHOST_WHITE GOLD GOLDENROD GRAY GREEN GREEN_YELLOW GREY HONEYDEW HOT_PINK"
                " INDIAN_RED INDIGO IVORY KHAKI LAVENDER LAVENDER_BLUSH LAWN_GREEN LEMON_CHIFFON LIGHT_BLUE"
                " LIGHT_CORAL LIGHT_CYAN LIGHT_GOLDENROD_YELLOW LIGHT_GRAY LIGHT_GREEN LIGHT_GREY LIGHT_PINK"
                " LIGHT_SALMON LIGHT_SEA_GREEN LIGHT_SKY_BLUE LIGHT_SLATE_GRAY LIGHT_SLATE_GREY LIGHT_STEEL_BLUE"
                " LIGHT_YELLOW LIME LIME_GREEN LINEN MAGENTA MAROON MEDIUM_AQUAMARINE MEDIUM_BLUE MEDIUM_ORCHID"
                " MEDIUM_PURPLE MEDIUM_SEA_GREEN MEDIUM_SLATE_BLUE MEDIUM_SPRING_GREEN MEDIUM_TURQUOISE"
                " MEDIUM_VIOLET_RED MIDNIGHT_BLUE MINT_CREAM MISTY_ROSE MOCCASIN NAVAJO_WHITE NAVY OLD_LACE"
                " OLIVE OLIVE_DRAB ORANGE ORANGE_RED ORCHID PALE_GOLDENROD PALE_GREEN PALE_TURQUOISE"
                " PALE_VIOLET_RED PAPAYA_WHIP PEACH_PUFF PERU PINK PLUM POWDER_BLUE PURPLE REBECCA_PURPLE"
                " RED ROSY_BROWN ROYAL_BLUE SADDLE_BROWN SALMON SANDY_BROWN SEASHELL SEA_GREEN SIENNA SILVER"
                " SKY_BLUE SLATE_BLUE SLATE_GRAY SLATE_GREY SNOW SPRING_GREEN STEEL_BLUE TAN TEAL THISTLE"
                " TOMATO TURQUOISE VIOLET WHEAT WHITE WHITE_SMOKE YELLOW YELLOW_GREEN ";
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
                fail("Unknown colour name; use an uppercase game colour such as SADDLE_BROWN.");
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

        void set_number(Json& object, const char* name, double value)
        {
            Json& target = member(object, name);
            if (!target.IsNumber() || target.GetDouble() != value) { target.SetDouble(value); }
        }

        Json rectangle_json(const Rect& rectangle, Allocator& allocator)
        {
            Json value(rapidjson::kObjectType);
            value.AddMember("x", rectangle.x, allocator);
            value.AddMember("y", rectangle.y, allocator);
            value.AddMember("width", rectangle.width, allocator);
            value.AddMember("height", rectangle.height, allocator);
            return value;
        }

        void write_rectangle(Json& object, const Rect& rectangle)
        {
            set_number(object, "x", rectangle.x);
            set_number(object, "y", rectangle.y);
            set_number(object, "width", rectangle.width);
            set_number(object, "height", rectangle.height);
        }

        void move_shape(Json& object, const Rect& rectangle)
        {
            if (string(object, "type") == "StructureRamp")
            {
                const Rect old = object_rectangle(object);
                Json& triangle = member(object, "triangle");
                for (int index = 1; index <= 3; ++index)
                {
                    const std::string x = "x" + std::to_string(index);
                    const std::string y = "y" + std::to_string(index);
                    set_number(triangle, x.c_str(), rectangle.x +
                        (number(triangle, x.c_str()) - old.x) / old.width * rectangle.width);
                    set_number(triangle, y.c_str(), rectangle.y +
                        (number(triangle, y.c_str()) - old.y) / old.height * rectangle.height);
                }
            }
            else if (object.HasMember("rectangle")) { write_rectangle(object["rectangle"], rectangle); }
            else
            {
                Json& value = member(object, "rectangle_tlbr");
                set_number(value, "left", rectangle.x);
                set_number(value, "top", rectangle.y);
                set_number(value, "right", rectangle.x + rectangle.width);
                set_number(value, "bottom", rectangle.y + rectangle.height);
            }
        }

        std::string escaped_key(std::string_view key)
        {
            std::string result;
            for (char character : key)
            {
                if (character == '~') { result += "~0"; }
                else if (character == '/') { result += "~1"; }
                else { result += character; }
            }
            return result;
        }

        void append_properties(const Json& value, const std::string& prefix,
            std::vector<Property>& properties)
        {
            for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
            {
                const std::string name(it->name.GetString(), it->name.GetStringLength());
                const std::string path = prefix + escaped_key(name);
                if (it->value.IsObject()) { append_properties(it->value, path + "/", properties); }
                else if (it->value.IsString())
                {
                    properties.push_back({ path, path,
                        std::string(it->value.GetString(), it->value.GetStringLength()), false, false });
                }
                else if (it->value.IsNumber() || it->value.IsBool())
                {
                    std::string text = json_text(it->value);
                    text.pop_back();
                    properties.push_back({ path, path, text, it->value.IsNumber(), it->value.IsBool() });
                }
            }
        }

        std::string object_label(const Json& object, std::size_t index)
        {
            if (object.HasMember("name") && object["name"].IsString())
            {
                return string(object, "name");
            }
            return string(object, "type") + " " + std::to_string(index + 1);
        }
    }

    struct LevelDocument::Impl
    {
        Document document;
        std::filesystem::path path;
        std::vector<std::string> history;
        std::size_t history_index = 0;
        std::string saved;

        template<typename Operation> void mutate(Operation operation)
        {
            Document candidate;
            candidate.CopyFrom(document, candidate.GetAllocator());
            operation(candidate);
            validate_document(candidate);
            if (candidate == document) { return; }
            const std::string snapshot = json_text(candidate);
            if (snapshot == history[history_index]) { return; }
            // Allocate the new history before swapping the live document.
            std::vector<std::string> next(history.begin(),
                history.begin() + static_cast<std::ptrdiff_t>(history_index + 1));
            next.push_back(snapshot);
            history.swap(next);
            history_index = history.size() - 1;
            document.Swap(candidate);
        }
    };

    LevelDocument::LevelDocument() : impl_(std::make_unique<Impl>()) { create(); }
    LevelDocument::~LevelDocument() = default;

    void LevelDocument::create()
    {
        Document document = parse(R"({
            "display_name":"New Stage", "icon_frame":"stage_close_quarters",
            "sound":{"sound_bank_name":"sound_bank_1","music_name":"holly_dolly","music_volume":0.3},
            "spawn_locations":{
                "team_a":[{"x":100,"y":1000},{"x":170,"y":1000},{"x":240,"y":1000},{"x":310,"y":1000}],
                "team_b":[{"x":2530,"y":1000},{"x":2600,"y":1000},{"x":2670,"y":1000},{"x":2740,"y":1000}]},
            "level_bounds":{
                "out_of_bounds":{"x":-200,"y":-200,"width":3400,"height":1800},
                "camera_bounds":{"x":0,"y":0,"width":3000,"height":1400},
                "zoom_out_start_bounds":{"x":540,"y":320,"width":1920,"height":1080},
                "zoom_out_finish_bounds":{"x":0,"y":0,"width":3000,"height":1400}},
            "collision_objects":[{"name":"floor","type":"StructurePaintable",
                "sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"GREY",
                "rectangle":{"x":0,"y":1200,"width":3000,"height":100},
                "collision_type":"STRUCTURE_PAINTABLE",
                "paintable_faces":{"left":false,"top":true,"right":false,"bottom":false}}],
            "non_collision_objects":[{"name":"background","type":"Visual",
                "sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"LIGHT_SKY_BLUE",
                "rectangle":{"x":-200,"y":-200,"width":3400,"height":1800}}]
        })");
        validate_document(document);
        auto next = std::make_unique<Impl>();
        next->history.push_back(json_text(document));
        next->document.Swap(document);
        impl_.swap(next);
    }

    void LevelDocument::load(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input) { fail("Could not open the level file."); }
        const std::string text{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
        if (input.bad()) { fail("Could not read the complete level file."); }
        Document document = parse(text);
        validate_document(document);
        auto next = std::make_unique<Impl>();
        next->path = path;
        next->saved = json_text(document);
        next->history.push_back(next->saved);
        next->document.Swap(document);
        impl_.swap(next);
    }

    void LevelDocument::save(const std::filesystem::path& path)
    {
        validate_document(impl_->document);
        if (path.empty()) { fail("Choose a level file before saving."); }
        if (std::filesystem::exists(path) && !std::filesystem::is_regular_file(path))
        {
            fail("The save destination is not a regular file.");
        }
        // Allocate document-state updates before committing any filesystem change.
        std::filesystem::path saved_path = path;
        std::string saved_text = serialize();
        // A sibling temporary guarantees a same-volume rename. CREATE_NEW owns it
        // exclusively, including when two editor processes save simultaneously.
        const std::string suffix = ".editor-" + std::to_string(GetCurrentProcessId()) + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::filesystem::path temporary = path;
        temporary += suffix + ".tmp";
        HANDLE output = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output == INVALID_HANDLE_VALUE)
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                "Could not create the temporary save file");
        }
        try
        {
            std::size_t offset = 0;
            while (offset < saved_text.size())
            {
                const DWORD count = static_cast<DWORD>(std::min(saved_text.size() - offset,
                    static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
                DWORD written = 0;
                if (!WriteFile(output, saved_text.data() + offset, count, &written, nullptr))
                {
                    throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                        "Could not write the complete level file");
                }
                if (written == 0) { fail("The temporary save file stopped accepting data."); }
                offset += written;
            }
            if (!FlushFileBuffers(output))
            {
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                    "Could not flush the level file");
            }
            const BOOL closed = CloseHandle(output);
            output = INVALID_HANDLE_VALUE;
            if (!closed)
            {
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                    "Could not close the level file");
            }
            // One Windows rename replaces the destination. Separate old->backup,
            // temp->destination renames leave the authored file absent between calls.
            // ReplaceFileW without a backup can also remove it on error 1176.
            if (!MoveFileExW(temporary.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                    "Could not replace the level file");
            }
        }
        catch (...)
        {
            if (output != INVALID_HANDLE_VALUE) { CloseHandle(output); }
            DeleteFileW(temporary.c_str());
            throw;
        }
        impl_->path.swap(saved_path);
        impl_->saved.swap(saved_text);
    }

    std::vector<Entry> LevelDocument::entries() const
    {
        const Document& document = impl_->document;
        std::vector<Entry> result{ { EntryKind::level, 0, "Level settings" } };
        for (std::size_t index = 0; index < bounds_names.size(); ++index)
        {
            result.push_back({ EntryKind::bounds, index, bounds_labels[index] });
        }
        for (const auto& [kind, name] : {
            std::pair{ EntryKind::spawn_a, "team_a" }, std::pair{ EntryKind::spawn_b, "team_b" } })
        {
            const Json& points = array(member(document, "spawn_locations"), name);
            for (rapidjson::SizeType index = 0; index < points.Size(); ++index)
            {
                result.push_back({ kind, index, std::string(kind == EntryKind::spawn_a ? "Team A spawn " :
                    "Team B spawn ") + std::to_string(index + 1) });
            }
        }
        for (const auto& [kind, name] : { std::pair{ EntryKind::collision, "collision_objects" },
            std::pair{ EntryKind::decoration, "non_collision_objects" } })
        {
            const Json& objects = array(document, name);
            for (rapidjson::SizeType index = 0; index < objects.Size(); ++index)
            {
                result.push_back({ kind, index, object_label(objects[index], index) });
            }
        }
        return result;
    }

    Rect LevelDocument::rectangle(const Entry& entry) const
    {
        const Json& value = entry_json(impl_->document, entry);
        if (entry.kind == EntryKind::level)
        {
            return plain_rectangle(member(member(value, "level_bounds"), "camera_bounds"));
        }
        if (entry.kind == EntryKind::bounds) { return plain_rectangle(value); }
        if (entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b)
        {
            return { number(value, "x"), number(value, "y"), 52.0, 120.0 };
        }
        return object_rectangle(value);
    }

    void LevelDocument::set_rectangle(const Entry& entry, const Rect& rectangle)
    {
        check_rectangle(rectangle);
        impl_->mutate([&](Document& document)
        {
            Json& value = entry_json(document, entry);
            if (entry.kind == EntryKind::level) { fail("Select a bounds rectangle to change the level bounds."); }
            if (entry.kind == EntryKind::bounds) { write_rectangle(value, rectangle); }
            else if (entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b)
            {
                set_number(value, "x", rectangle.x);
                set_number(value, "y", rectangle.y);
            }
            else { move_shape(value, rectangle); }
        });
    }

    std::vector<Property> LevelDocument::properties(const Entry& entry) const
    {
        const Json& value = entry_json(impl_->document, entry);
        std::vector<Property> result;
        if (entry.kind == EntryKind::level)
        {
            // Bounds and object arrays have their own canvas selections.
            Document metadata;
            metadata.CopyFrom(value, metadata.GetAllocator());
            metadata.RemoveMember("level_bounds");
            metadata.RemoveMember("spawn_locations");
            metadata.RemoveMember("collision_objects");
            metadata.RemoveMember("non_collision_objects");
            append_properties(metadata, "", result);
        }
        else { append_properties(value, "", result); }
        return result;
    }

    void LevelDocument::set_property(const Entry& entry, const std::string& path,
        const std::string& value)
    {
        impl_->mutate([&](Document& document)
        {
            Json& object = entry_json(document, entry);
            const std::string pointer_text = path.starts_with('/') ? path : "/" + path;
            const rapidjson::Pointer pointer(pointer_text.c_str());
            if (!pointer.IsValid()) { fail("Invalid property path."); }
            Json* target = pointer.Get(object);
            if (target == nullptr) { fail("The selected property no longer exists."); }
            if (target->IsString())
            {
                target->SetString(value.data(), static_cast<rapidjson::SizeType>(value.size()),
                    document.GetAllocator());
            }
            else if (target->IsBool())
            {
                if (value != "true" && value != "false") { fail("Use true or false for this property."); }
                target->SetBool(value == "true");
            }
            else if (target->IsNumber())
            {
                Document parsed = parse(value);
                if (!parsed.IsNumber() || !std::isfinite(parsed.GetDouble()))
                {
                    fail("This property needs a finite number.");
                }
                target->CopyFrom(parsed, document.GetAllocator());
            }
            else { fail("Only text, numbers and true/false properties can be edited here."); }
        });
    }

    Entry LevelDocument::add_object(const std::string& type, const Rect& rectangle)
    {
        check_rectangle(rectangle);
        Entry added;
        impl_->mutate([&](Document& document)
        {
            Allocator& allocator = document.GetAllocator();
            Json value(rapidjson::kObjectType);
            if (type == "SpawnA" || type == "SpawnB")
            {
                added.kind = type == "SpawnA" ? EntryKind::spawn_a : EntryKind::spawn_b;
                value.AddMember("x", rectangle.x, allocator);
                value.AddMember("y", rectangle.y, allocator);
            }
            else
            {
                const bool visual = type == "Visual";
                const bool ramp_right = type == "StructureRampRight";
                const bool ramp_left = type == "StructureRampLeft";
                const bool paintable = type == "StructurePaintable";
                const bool jump = type == "StructureJumpThrough";
                if (!visual && !ramp_right && !ramp_left && !paintable && !jump && type != "Structure")
                {
                    fail("Unknown object palette entry: " + type);
                }
                added.kind = visual ? EntryKind::decoration : EntryKind::collision;
                const char* object_type = visual ? "Visual" : (ramp_left || ramp_right ?
                    "StructureRamp" : (paintable ? "StructurePaintable" : "Structure"));
                value.AddMember("name", Json(type.c_str(), allocator), allocator);
                value.AddMember("type", Json(object_type, allocator), allocator);
                value.AddMember("sheet_name", "sprite_sheet_1", allocator);
                value.AddMember("frame_name", Json(ramp_left || ramp_right ? "ramp_right_200_100" :
                    "square_white_4", allocator), allocator);
                value.AddMember("colour", Json(visual ? "WHITE" : (paintable ? "GREY" :
                    (jump ? "TAN" : "SADDLE_BROWN")), allocator), allocator);
                if (ramp_left || ramp_right)
                {
                    Json triangle(rapidjson::kObjectType);
                    triangle.AddMember("x1", ramp_right ? rectangle.x : rectangle.x + rectangle.width, allocator);
                    triangle.AddMember("y1", rectangle.y + rectangle.height, allocator);
                    triangle.AddMember("x2", ramp_right ? rectangle.x + rectangle.width : rectangle.x, allocator);
                    triangle.AddMember("y2", rectangle.y + rectangle.height, allocator);
                    triangle.AddMember("x3", ramp_right ? rectangle.x + rectangle.width : rectangle.x, allocator);
                    triangle.AddMember("y3", rectangle.y, allocator);
                    value.AddMember("triangle", triangle, allocator);
                }
                else { value.AddMember("rectangle", rectangle_json(rectangle, allocator), allocator); }
                if (!visual)
                {
                    const char* collision = ramp_right ? "STRUCTURE_RAMP_RIGHT" : (ramp_left ?
                        "STRUCTURE_RAMP_LEFT" : (paintable ? "STRUCTURE_PAINTABLE" :
                            (jump ? "STRUCTURE_JUMP_THROUGH" : "STRUCTURE")));
                    value.AddMember("collision_type", Json(collision, allocator), allocator);
                }
                if (paintable)
                {
                    Json faces(rapidjson::kObjectType);
                    for (const char* face : { "left", "top", "right", "bottom" })
                    {
                        faces.AddMember(Json(face, allocator), Json(true), allocator);
                    }
                    value.AddMember("paintable_faces", faces, allocator);
                }
            }
            Json& objects = entry_array(document, added.kind);
            added.index = objects.Size();
            objects.PushBack(value, allocator);
        });
        for (const Entry& entry : entries())
        {
            if (entry.kind == added.kind && entry.index == added.index) { return entry; }
        }
        return added;
    }

    Entry LevelDocument::duplicate(const Entry& entry)
    {
        Entry added = entry;
        impl_->mutate([&](Document& document)
        {
            Json& objects = entry_array(document, entry.kind);
            Json value;
            value.CopyFrom(entry_json(document, entry), document.GetAllocator());
            if (entry.kind == EntryKind::collision || entry.kind == EntryKind::decoration)
            {
                Rect rectangle = this->rectangle(entry);
                rectangle.x += 50.0;
                rectangle.y += 50.0;
                move_shape(value, rectangle);
                if (value.HasMember("name") && value["name"].IsString())
                {
                    const std::string name = string(value, "name") + " copy";
                    value["name"].SetString(name.c_str(), document.GetAllocator());
                    added.label = name;
                }
            }
            added.index = objects.Size();
            objects.PushBack(value, document.GetAllocator());
        });
        for (const Entry& candidate : entries())
        {
            if (candidate.kind == added.kind && candidate.index == added.index) { return candidate; }
        }
        return added;
    }

    void LevelDocument::erase(const Entry& entry)
    {
        impl_->mutate([&](Document& document)
        {
            entry_json(document, entry);
            Json& objects = entry_array(document, entry.kind);
            objects.Erase(objects.Begin() + static_cast<std::ptrdiff_t>(entry.index));
        });
    }

    bool LevelDocument::undo()
    {
        if (!can_undo()) { return false; }
        Document previous = parse(impl_->history[impl_->history_index - 1]);
        impl_->document.Swap(previous);
        --impl_->history_index;
        return true;
    }

    bool LevelDocument::redo()
    {
        if (!can_redo()) { return false; }
        Document next = parse(impl_->history[impl_->history_index + 1]);
        impl_->document.Swap(next);
        ++impl_->history_index;
        return true;
    }

    bool LevelDocument::can_undo() const { return impl_->history_index > 0; }
    bool LevelDocument::can_redo() const { return impl_->history_index + 1 < impl_->history.size(); }
    bool LevelDocument::dirty() const { return impl_->history[impl_->history_index] != impl_->saved; }
    const std::filesystem::path& LevelDocument::path() const { return impl_->path; }
    std::string LevelDocument::serialize() const { return json_text(impl_->document); }

    std::vector<std::string> LevelDocument::validate() const
    {
        try { validate_document(impl_->document); }
        catch (const std::exception& error) { return { error.what() }; }
        return {};
    }
}
