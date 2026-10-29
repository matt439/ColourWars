#include "editor/level_document.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "editor/content_catalog.h"
#include "editor/level_json.h"
#include "editor/horizontal_authoring.h"
#include "editor/slab_authoring.h"
#include "game/level_format.h"
#include <rapidjson/pointer.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

namespace colourwars_editor
{
    using namespace level_json;

    namespace
    {
        constexpr std::array<const char*, 4> bounds_labels = {
            "Out of bounds", "Camera bounds", "Zoom start", "Zoom finish" };

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
            else if (entry.kind == EntryKind::station)
            {
                if (!document.HasMember("stations")) { fail("The selected entry no longer exists."); }
                values = &member(document, "stations");
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
            if (kind == EntryKind::station)
            {
                // A level without stations gains the optional array on its first.
                if (!document.HasMember("stations"))
                {
                    document.AddMember("stations", Json(rapidjson::kArrayType), document.GetAllocator());
                }
                return member(document, "stations");
            }
            if (kind == EntryKind::collision || kind == EntryKind::decoration)
            {
                return member(document,
                    kind == EntryKind::collision ? "collision_objects" : "non_collision_objects");
            }
            fail("Level metadata and bounds cannot be duplicated or deleted.");
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
            if (string(object, "type") == "FixedSlabPaintable")
            { move_slab(object, rectangle); return; }
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
                // Keep an empty first token distinct from an absolute root field.
                const std::string path = prefix.empty() && name.empty() ? "/" : prefix + escaped_key(name);
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

        bool is_point(EntryKind kind)
        {
            return kind == EntryKind::spawn_a || kind == EntryKind::spawn_b || kind == EntryKind::station;
        }

        // The first of base, base_2, base_3, ... (base_1, base_2, ... when
        // numbered) that no station in the document already uses.
        std::string unused_station_name(const Document& document, const std::string& base, bool numbered)
        {
            std::vector<std::string> names;
            if (document.HasMember("stations") && document["stations"].IsArray())
            {
                for (const Json& station : document["stations"].GetArray())
                {
                    if (station.IsObject() && station.HasMember("name") && station["name"].IsString())
                    {
                        names.emplace_back(station["name"].GetString(), station["name"].GetStringLength());
                    }
                }
            }
            for (std::size_t number = 1;; ++number)
            {
                std::string name = numbered || number > 1 ? base + "_" + std::to_string(number) : base;
                if (std::find(names.begin(), names.end(), name) == names.end()) { return name; }
            }
        }

        std::filesystem::path save_target(const std::filesystem::path& path)
        {
            return std::filesystem::absolute(path).lexically_normal().make_preferred();
        }

        bool same_save_target(const std::filesystem::path& path,
            const std::filesystem::path& baseline)
        {
            const std::filesystem::path target = save_target(path);
            if (CompareStringOrdinal(target.c_str(), -1, baseline.c_str(), -1, TRUE) == CSTR_EQUAL)
            { return true; }
            std::error_code error;
            return std::filesystem::equivalent(target, baseline, error) && !error;
        }

        void check_saved_source(const std::filesystem::path& path, const std::string& baseline)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input)
            {
                fail("Could not read the saved level file. It may have been removed or opened exclusively. "
                    "Use Save As to preserve your edits in another file.");
            }
            const std::string text{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
            if (input.bad())
            {
                fail("Could not read the complete saved level file. "
                    "Use Save As to preserve your edits in another file.");
            }
            if (text != baseline)
            {
                fail("The level file has changed outside this editor since it was loaded or saved. "
                    "Use Save As to preserve your edits in another file.");
            }
        }
    }

    struct LevelDocument::Impl
    {
        Document document;
        std::filesystem::path path;
        std::filesystem::path motion_tuning;
        std::vector<std::string> history;
        std::size_t history_index = 0;
        std::string saved;
        std::filesystem::path source_path;
        std::string source_bytes;

        template<typename Operation> void mutate(Operation operation)
        {
            Document candidate;
            candidate.CopyFrom(document, candidate.GetAllocator());
            operation(candidate);
            validate_document(candidate, motion_tuning);
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

    LevelDocument::LevelDocument() : LevelDocument(std::filesystem::path{}) {}
    LevelDocument::LevelDocument(const std::filesystem::path& motion_tuning) : impl_(std::make_unique<Impl>())
    {
        impl_->motion_tuning = motion_tuning;
        create();
    }
    LevelDocument::~LevelDocument() = default;

    void LevelDocument::create()
    {
        Document document = parse(R"({
            "display_name":"New Stage", "icon_frame":"stage_close_quarters",
            "sound":{"sound_bank_name":"paint_exe","music_name":"music.close_quarters","music_volume":0.65,
                "end_sting_name":"music.end_sting.close_quarters","end_sting_volume":0.65},
            "spawn_locations":{
                "team_a":[{"x":100,"y":1000},{"x":170,"y":1000},{"x":240,"y":1000},{"x":310,"y":1000}],
                "team_b":[{"x":2530,"y":1000},{"x":2600,"y":1000},{"x":2670,"y":1000},{"x":2740,"y":1000}]},
            "level_bounds":{
                "out_of_bounds":{"x":-200,"y":-200,"width":3400,"height":1800},
                "camera_bounds":{"x":0,"y":0,"width":3000,"height":1400},
                "zoom_out_start_bounds":{"x":540,"y":320,"width":1920,"height":1080},
                "zoom_out_finish_bounds":{"x":0,"y":0,"width":3000,"height":1400}},
            "collision_objects":[{"name":"floor","type":"StructurePaintable",
                "sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"GAINSBORO",
                "rectangle":{"x":0,"y":1200,"width":3000,"height":100},
                "collision_type":"STRUCTURE_PAINTABLE","material":"ground",
                "paintable_faces":{"left":false,"top":true,"right":false,"bottom":false}}],
            "non_collision_objects":[{"name":"background","type":"Visual",
                "sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"404040",
                "rectangle":{"x":-200,"y":-200,"width":3400,"height":1800}}]
        })");
        document.AddMember("format", level_format::current, document.GetAllocator());
        validate_document(document, impl_->motion_tuning);
        auto next = std::make_unique<Impl>();
        next->motion_tuning = impl_->motion_tuning;
        next->history.push_back(json_text(document));
        next->document.Swap(document);
        impl_.swap(next);
    }

    void LevelDocument::load(const std::filesystem::path& path)
    {
        const std::u8string filename = path.u8string();
        const std::string source = "Level file '" +
            std::string(reinterpret_cast<const char*>(filename.data()), filename.size()) + "': ";
        std::ifstream input(path, std::ios::binary);
        if (!input) { fail(source + "Could not open the level file."); }
        const std::string text{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
        if (input.bad()) { fail(source + "Could not read the complete level file."); }
        Document document;
        try
        {
            document = parse(text);
            validate_document(document, impl_->motion_tuning);
        }
        catch (const std::exception& error) { fail(source + error.what()); }
        auto next = std::make_unique<Impl>();
        next->motion_tuning = impl_->motion_tuning;
        next->path = path;
        next->source_path = save_target(path);
        next->source_bytes = text;
        next->saved = json_text(document);
        next->history.push_back(next->saved);
        next->document.Swap(document);
        impl_.swap(next);
    }

    void LevelDocument::save(const std::filesystem::path& path)
    {
        validate_document(impl_->document, impl_->motion_tuning);
        if (path.empty()) { fail("Choose a level file before saving."); }
        if (std::filesystem::exists(path) && !std::filesystem::is_regular_file(path))
        {
            fail("The save destination is not a regular file.");
        }
        const bool replacing_source = !impl_->path.empty() && same_save_target(path, impl_->source_path);
        if (replacing_source) { check_saved_source(path, impl_->source_bytes); }
        // Allocate document-state updates before committing any filesystem change.
        // Saving upgrades a legacy file, but a failed write must leave even
        // its omitted format field and undo/redo history untouched.
        Document saved_document;
        saved_document.CopyFrom(impl_->document, saved_document.GetAllocator());
        if (!saved_document.HasMember("format"))
        {
            saved_document.AddMember("format", level_format::current, saved_document.GetAllocator());
        }
        std::filesystem::path saved_path = path;
        std::filesystem::path source_path = save_target(path);
        std::string saved_text = json_text(saved_document);
        std::string source_bytes = saved_text;
        std::vector<std::string> saved_history = impl_->history;
        saved_history[impl_->history_index] = saved_text;
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
            // Another writer may have changed the destination while the temporary
            // was written. Refuse before replacement, retaining both authors' work.
            if (replacing_source) { check_saved_source(path, impl_->source_bytes); }
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
        impl_->document.Swap(saved_document);
        impl_->history.swap(saved_history);
        impl_->path.swap(saved_path);
        impl_->saved.swap(saved_text);
        impl_->source_path.swap(source_path);
        impl_->source_bytes.swap(source_bytes);
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
        if (document.HasMember("stations"))
        {
            const Json& stations = array(document, "stations");
            for (rapidjson::SizeType index = 0; index < stations.Size(); ++index)
            {
                result.push_back({ EntryKind::station, index, "Station " + string(stations[index], "name") });
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
        // Spawns and stations are the character's top-left.
        if (is_point(entry.kind)) { return { number(value, "x"), number(value, "y"), 52.0, 120.0 }; }
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
            else if (is_point(entry.kind))
            {
                // A station's lesson area stays put; the shared rule refuses a
                // move that leaves it.
                set_number(value, "x", rectangle.x);
                set_number(value, "y", rectangle.y);
            }
            else { move_shape(value, rectangle); }
        });
    }

    void LevelDocument::align_origin(const Entry& entry, int grid)
    {
        if (grid <= 0) { fail("Grid spacing must be positive."); }
        if (entry.kind == EntryKind::level) { fail("Select geometry before aligning its origin."); }
        const Json& value = entry_json(impl_->document, entry);
        const bool fixed_slab = entry.kind == EntryKind::collision && string(value, "type") == "FixedSlabPaintable";
        Rect aligned;
        if (fixed_slab)
        {
            const Json& origin = member(value, "slab");
            aligned.x = number(origin, "x"); aligned.y = number(origin, "y");
        }
        else { aligned = rectangle(entry); }
        const double x = std::round(aligned.x / grid) * grid;
        const double y = std::round(aligned.y / grid) * grid;
        if (x == aligned.x && y == aligned.y) { return; }
        aligned.x = x; aligned.y = y;
        impl_->mutate([&](Document& document)
        {
            Json& object = entry_json(document, entry);
            if (fixed_slab)
            {
                Json& origin = member(object, "slab");
                set_number(origin, "x", x); set_number(origin, "y", y);
            }
            else if (entry.kind == EntryKind::bounds) { write_rectangle(object, aligned); }
            else if (is_point(entry.kind))
            {
                set_number(object, "x", x); set_number(object, "y", y);
            }
            else { move_shape(object, aligned); }
        });
    }

    std::optional<MotionGuide> LevelDocument::motion_guide(const Entry& entry,
        const std::optional<Rect>& preview) const
    {
        if (entry.kind != EntryKind::collision) { return std::nullopt; }
        const Json& object = entry_json(impl_->document, entry);
        if (!object.HasMember("motion")) { return std::nullopt; }
        return horizontal_motion_guide(object, load_motion_player_size(impl_->motion_tuning), preview);
    }

    std::optional<PaintPolygon> LevelDocument::slab_guide(const Entry& entry,
        const std::optional<Rect>& preview) const
    {
        if (entry.kind != EntryKind::collision) { return std::nullopt; }
        const Json& object = entry_json(impl_->document, entry);
        if (string(object, "type") != "FixedSlabPaintable") { return std::nullopt; }
        if (!preview) { return decode_slab(object).corners; }
        Document candidate;
        candidate.CopyFrom(object, candidate.GetAllocator());
        move_slab(candidate, *preview);
        return decode_slab(candidate).corners;
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
            // Show the runtime default without changing the source document.
            if (!metadata.HasMember("include_in_random"))
            {
                metadata.AddMember("include_in_random", true, metadata.GetAllocator());
            }
            append_properties(metadata, "", result);
        }
        else if (entry.kind == EntryKind::station)
        {
            // The lesson block is preserved as written, with no controls.
            Document fields;
            fields.CopyFrom(value, fields.GetAllocator());
            fields.RemoveMember("lesson");
            append_properties(fields, "", result);
            for (Property& property : result)
            {
                if (property.path == "name") { property.label = "name (a-z, 0-9 and _; unique)"; }
            }
        }
        else { append_properties(value, "", result); }
        if (entry.kind == EntryKind::collision && value.HasMember("motion"))
        {
            for (Property& property : result)
            {
                if (property.path == "motion/type") { property.label = "motion/type (horizontal)"; }
                else if (property.path == "motion/travel_x") { property.label = "motion/travel_x (world units)"; }
                else if (property.path == "motion/period") { property.label = "motion/period (round-trip seconds)"; }
                else if (property.path == "motion/phase") { property.label = "motion/phase (cycles [0,1))"; }
                else if (property.path == "motion/drive") { property.label = "motion/drive (cycle or paint)"; }
                else if (property.path == "motion/team_a_heads")
                { property.label = "motion/team_a_heads (right or left)"; }
            }
            const Json& motion = value["motion"];
            const bool paint = motion.IsObject() && motion.HasMember("drive") && motion["drive"].IsString() &&
                string(motion, "drive") == "paint";
            if (paint)
            {
                for (Property& property : result)
                {
                    if (property.path == "motion/phase")
                    { property.label = "motion/phase (paint: 0 left end to 0.5 right end)"; }
                }
            }
            // An absent drive is the runtime's cycle; show it so the drive can
            // be switched without hand-editing the file.
            if (motion.IsObject() && !motion.HasMember("drive"))
            { result.push_back({ "motion/drive", "motion/drive (cycle or paint)", "cycle", false, false }); }
        }
        if (entry.kind == EntryKind::collision && value.HasMember("gate"))
        {
            for (Property& property : result)
            {
                if (property.path == "gate") { property.label = "gate (paint: open to the team holding more of it)"; }
            }
        }
        if (entry.kind == EntryKind::collision && value.HasMember("collision_type") &&
            value["collision_type"].IsString())
        {
            // material is optional and open-ended in the inspector's string edit,
            // so name the values this collision tag accepts; an empty value is the
            // legacy stretched draw and applying one adds the field.
            const std::string allowed = allowed_materials(string(value, "collision_type"));
            const auto existing = std::find_if(result.begin(), result.end(),
                [](const Property& property) { return property.path == "material"; });
            if (existing != result.end()) { existing->label = "material (" +
                (string(value, "type") == "FixedSlabPaintable" ? "eraser" : allowed) + ")"; }
            else if (!allowed.empty())
            {
                result.push_back({ "material", "material (" + allowed + ")", "", false, false });
            }
        }
        return result;
    }

    void LevelDocument::set_property(const Entry& entry, const std::string& path,
        const std::string& value)
    {
        impl_->mutate([&](Document& document)
        {
            Json& object = entry_json(document, entry);
            const std::string pointer_text = path.starts_with('/') ? path : "/" + path;
            const rapidjson::Pointer pointer(pointer_text.data(), pointer_text.size());
            if (!pointer.IsValid()) { fail("Invalid property path."); }
            if (entry.kind == EntryKind::station &&
                (pointer_text == "/lesson" || pointer_text.starts_with("/lesson/")))
            {
                fail("A station's lesson is kept as written; edit it in the level file.");
            }
            Json* target = pointer.Get(object);
            if (target == nullptr && entry.kind == EntryKind::level &&
                pointer_text == "/include_in_random")
            {
                if (value != "true" && value != "false") { fail("Use true or false for this property."); }
                object.AddMember("include_in_random", value == "true", document.GetAllocator());
                return;
            }
            if (pointer_text == "/gate" && entry.kind == EntryKind::collision && value.empty())
            {
                // Clearing the gate leaves an ordinary paintable block.
                object.RemoveMember("gate");
                return;
            }
            if (pointer_text == "/material" && entry.kind == EntryKind::collision)
            {
                // Clearing the material returns the object to the legacy draw.
                if (value.empty()) { object.RemoveMember("material"); return; }
                if (target == nullptr)
                {
                    object.AddMember("material",
                        Json(value.data(), static_cast<rapidjson::SizeType>(value.size()), document.GetAllocator()),
                        document.GetAllocator());
                    return;
                }
            }
            if (entry.kind == EntryKind::collision && object.HasMember("motion") && object["motion"].IsObject() &&
                (pointer_text == "/motion/drive" || pointer_text == "/motion/team_a_heads"))
            {
                // A drive switch carries its heading with it: paint gains team A
                // heading right unless it already has one, and cycle (or a
                // cleared drive, which is the cycle default) sheds it. Anything
                // else is written as typed and the shared drive rule refuses it.
                Json& motion = object["motion"];
                Allocator& allocator = document.GetAllocator();
                const auto put = [&](const char* key, const std::string& text)
                {
                    if (motion.HasMember(key)) { motion.RemoveMember(key); }
                    motion.AddMember(Json(key, allocator),
                        Json(text.data(), static_cast<rapidjson::SizeType>(text.size()), allocator), allocator);
                };
                if (pointer_text == "/motion/team_a_heads")
                {
                    if (value.empty()) { motion.RemoveMember("team_a_heads"); }
                    else if (target != nullptr && target->IsString())
                    { target->SetString(value.data(), static_cast<rapidjson::SizeType>(value.size()), allocator); }
                    else { put("team_a_heads", value); }
                    return;
                }
                if (value.empty() || (value == "cycle" && target == nullptr))
                {
                    motion.RemoveMember("drive");
                    motion.RemoveMember("team_a_heads");
                    return;
                }
                if (target != nullptr && target->IsString())
                { target->SetString(value.data(), static_cast<rapidjson::SizeType>(value.size()), allocator); }
                else { put("drive", value); }
                if (value == "cycle") { motion.RemoveMember("team_a_heads"); }
                if (value == "paint" && !motion.HasMember("team_a_heads")) { put("team_a_heads", "right"); }
                return;
            }
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
            else if (type == "Station")
            {
                added.kind = EntryKind::station;
                const std::string name = unused_station_name(document, "station", true);
                value.AddMember("name", Json(name.data(), static_cast<rapidjson::SizeType>(name.size()), allocator),
                    allocator);
                value.AddMember("x", rectangle.x, allocator);
                value.AddMember("y", rectangle.y, allocator);
            }
            else if (type == "FixedSlabRight" || type == "FixedSlabLeft")
            {
                // The palette position is the left top-edge endpoint. Width
                // and height are deliberately not authored for this fixed shape.
                added.kind = EntryKind::collision;
                value.AddMember("name", Json(type.c_str(), allocator), allocator);
                value.AddMember("type", "FixedSlabPaintable", allocator);
                value.AddMember("collision_type", "STRUCTURE_FIXED_SLAB", allocator);
                value.AddMember("material", "eraser", allocator);
                Json slab(rapidjson::kObjectType);
                slab.AddMember("x", rectangle.x, allocator); slab.AddMember("y", rectangle.y, allocator);
                slab.AddMember("rise", Json(type == "FixedSlabRight" ? "right" : "left", allocator), allocator);
                value.AddMember("slab", slab, allocator);
                Json faces(rapidjson::kObjectType);
                for (const char* face : { "left", "top", "right", "bottom" })
                    faces.AddMember(Json(face, allocator), Json(true), allocator);
                value.AddMember("paintable_faces", faces, allocator);
            }
            else
            {
                const bool visual = type == "Visual";
                const bool ramp_right = type == "StructureRampRight";
                const bool ramp_left = type == "StructureRampLeft";
                const bool moving = type == "MovingPaintable" || type == "MovingJumpThrough";
                const bool gate = type == "TeamGate";
                const bool paintable = type == "StructurePaintable" || moving || gate;
                const bool jump = type == "StructureJumpThrough" || type == "MovingJumpThrough";
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
                value.AddMember("colour", Json(visual ? "WHITE" : (paintable ? "GAINSBORO" :
                    (jump ? "A0A0A0" : "808080")), allocator), allocator);
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
                        "STRUCTURE_RAMP_LEFT" : (jump ? "STRUCTURE_JUMP_THROUGH" :
                            (paintable ? "STRUCTURE_PAINTABLE" : "STRUCTURE")));
                    value.AddMember("collision_type", Json(collision, allocator), allocator);
                    // Each kind starts drawn as its usual terrain-atlas material.
                    const char* material = ramp_left || ramp_right ? "set_square" :
                        (jump ? "ruler" : (gate ? "eraser" : (paintable ? "pencil" : "ground")));
                    value.AddMember("material", Json(material, allocator), allocator);
                }
                if (paintable)
                {
                    Json faces(rapidjson::kObjectType);
                    for (const char* face : { "left", "top", "right", "bottom" })
                    {
                        faces.AddMember(Json(face, allocator), Json(!gate || std::string(face) != "bottom"), allocator);
                    }
                    value.AddMember("paintable_faces", faces, allocator);
                }
                if (gate) { value.AddMember("gate", "paint", allocator); }
                if (moving)
                {
                    Json motion(rapidjson::kObjectType);
                    motion.AddMember("type", "horizontal", allocator);
                    motion.AddMember("travel_x", 120.0, allocator);
                    motion.AddMember("period", 6.4, allocator);
                    motion.AddMember("phase", 0.0, allocator);
                    value.AddMember("motion", motion, allocator);
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
                if (value.HasMember("motion"))
                {
                    // A copy retains horizontal travel/phase and moves down by
                    // the whole source body plus standing corridor, not +50
                    // into the original sweep. Final validation checks all
                    // other geometry and bounds before publishing the copy.
                    const auto source = decode_horizontal_motion(value);
                    const auto player = load_motion_player_size(impl_->motion_tuning);
                    const auto source_path = horizontal_motion_contract::path(source);
                    auto copy = source;
                    copy.anchor.y += copy.anchor.height + player.height;
                    for (int adjustment = 0; adjustment < 4; ++adjustment)
                    {
                        const auto standing = horizontal_motion_contract::standing(
                            horizontal_motion_contract::path(copy), player.width, player.height);
                        if (standing.top >= source_path.bottom) { break; }
                        copy.anchor.y = std::nextafter(static_cast<float>(static_cast<double>(copy.anchor.y) +
                            source_path.bottom - standing.top), std::numeric_limits<float>::infinity());
                    }
                    rectangle.y = copy.anchor.y;
                }
                else if (string(value, "type") == "FixedSlabPaintable")
                {
                    // A 50/50 offset leaves mirrored 24-normal-thick slabs
                    // overlapped. Put this fixed shape below its whole body;
                    // validation still refuses a collision with another slab.
                    rectangle.x += 50.0; rectangle.y += rectangle.height + 50.0;
                }
                else { rectangle.x += 50.0; rectangle.y += 50.0; }
                move_shape(value, rectangle);
                if (value.HasMember("name") && value["name"].IsString())
                {
                    const std::string name = string(value, "name") + " copy";
                    value["name"].SetString(name.data(), static_cast<rapidjson::SizeType>(name.size()),
                        document.GetAllocator());
                    added.label = name;
                }
            }
            else if (entry.kind == EntryKind::station)
            {
                // Station names must stay typeable and unique, so the copy
                // takes _copy rather than " copy". Its lesson stays with the
                // original; a copy would repeat its words over another area.
                set_number(value, "x", number(value, "x") + 50.0);
                set_number(value, "y", number(value, "y") + 50.0);
                value.RemoveMember("lesson");
                const std::string name = unused_station_name(document, string(value, "name") + "_copy", false);
                value["name"].SetString(name.data(), static_cast<rapidjson::SizeType>(name.size()),
                    document.GetAllocator());
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

    namespace
    {
        void set_text(Json& object, const char* name, const std::string& text, Allocator& allocator)
        {
            member(object, name).SetString(text.data(), static_cast<rapidjson::SizeType>(text.size()), allocator);
        }

        // A cycle (mover or drift) half a period on is its own reflection.
        // Rounding to twelve places keeps an authored decimal such as 0.51
        // a short decimal (0.01), so the saved text reparses to the same value.
        void advance_half_cycle(Json& fields)
        {
            double phase = number(fields, "phase") + 0.5;
            if (phase >= 1.0) { phase -= 1.0; }
            phase = std::round(phase * 1e12) / 1e12;
            // A phase just under 0.5 can round up to a whole cycle, which is 0.
            if (phase >= 1.0) { phase -= 1.0; }
            set_number(fields, "phase", phase);
        }

        // Reflect one object record across x = sum / 2, where sum is the
        // out_of_bounds left + right. The game reads hand from the ramp's
        // collision type and the slab's rise, so those change with the shape.
        void mirror_object(Json& object, double sum, Allocator& allocator)
        {
            const std::string type = string(object, "type");
            if (type == "FixedSlabPaintable")
            {
                Json& slab = member(object, "slab");
                const std::string rise = string(slab, "rise");
                if (rise != "left" && rise != "right") { fail("slab.rise must be left or right."); }
                // The stored endpoint is the left one; the far end becomes it.
                set_number(slab, "x", sum - number(slab, "x") - 160.0);
                set_number(slab, "y", number(slab, "y") + (rise == "right" ? -80.0 : 80.0));
                set_text(slab, "rise", rise == "right" ? "left" : "right", allocator);
            }
            else if (type == "StructureRamp")
            {
                Json& triangle = member(object, "triangle");
                for (const char* x : { "x1", "x2", "x3" }) { set_number(triangle, x, sum - number(triangle, x)); }
                const std::string collision = string(object, "collision_type");
                if (collision == "STRUCTURE_RAMP_RIGHT") { set_text(object, "collision_type", "STRUCTURE_RAMP_LEFT", allocator); }
                else if (collision == "STRUCTURE_RAMP_LEFT") { set_text(object, "collision_type", "STRUCTURE_RAMP_RIGHT", allocator); }
            }
            else
            {
                Rect rectangle = object_rectangle(object);
                double reach = rectangle.width;
                if (object.HasMember("motion"))
                {
                    Json& motion = member(object, "motion");
                    if (motion.IsObject() && motion.HasMember("drive") && motion["drive"].IsString() &&
                        string(motion, "drive") == "paint")
                    {
                        fail("Mirror copy does not copy paint ferries yet: their heading and paint-drive phase "
                            "need their own rule. Duplicate it and set the copy by hand.");
                    }
                    // The rectangle is the leftmost anchor, so reflect the whole path.
                    reach += number(motion, "travel_x");
                    advance_half_cycle(motion);
                }
                if (object.HasMember("drift") && object["drift"].IsObject()) { advance_half_cycle(object["drift"]); }
                rectangle.x = sum - rectangle.x - reach;
                move_shape(object, rectangle);
            }
            if (object.HasMember("paintable_faces") && object["paintable_faces"].IsObject())
            {
                Json& faces = object["paintable_faces"];
                const bool left = faces.HasMember("left"), right = faces.HasMember("right");
                if (left && right) { faces["left"].Swap(faces["right"]); }
                else if (left || right)
                {
                    Json value; value.CopyFrom(faces[left ? "left" : "right"], allocator);
                    faces.RemoveMember(left ? "left" : "right");
                    faces.AddMember(Json(left ? "right" : "left", allocator), value, allocator);
                }
            }
            if (object.HasMember("name") && object["name"].IsString())
            {
                std::string name = string(object, "name");
                if (name.ends_with("_a")) { name.back() = 'b'; }
                else if (name.ends_with("_b")) { name.back() = 'a'; }
                else { name += " mirror"; }
                set_text(object, "name", name, allocator);
            }
        }
    }

    Entry LevelDocument::mirror_copy(const Entry& entry)
    {
        if (entry.kind == EntryKind::level || entry.kind == EntryKind::bounds)
        { fail("Select an object or spawn to mirror; level settings and bounds have no mirror copy."); }
        if (entry.kind == EntryKind::station)
        { fail("Stations have no mirror copy: each names one place. Duplicate it and move the copy."); }
        Entry added = entry;
        impl_->mutate([&](Document& document)
        {
            Allocator& allocator = document.GetAllocator();
            const Rect world = plain_rectangle(member(member(document, "level_bounds"), "out_of_bounds"));
            const double sum = world.x + (world.x + world.width);
            Json value;
            value.CopyFrom(entry_json(document, entry), allocator);
            if (entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b)
            {
                // A spawn point is the body's top-left, so reflect the whole
                // character from source tuning (the editor's 52-unit box without it).
                const double width = impl_->motion_tuning.empty() ? this->rectangle(entry).width :
                    static_cast<double>(load_motion_player_size(impl_->motion_tuning).width);
                set_number(value, "x", sum - number(value, "x") - width);
                added.kind = entry.kind == EntryKind::spawn_a ? EntryKind::spawn_b : EntryKind::spawn_a;
            }
            else { mirror_object(value, sum, allocator); }
            Json& objects = entry_array(document, added.kind);
            added.index = objects.Size();
            objects.PushBack(value, allocator);
        });
        for (const Entry& candidate : entries())
        {
            if (candidate.kind == added.kind && candidate.index == added.index) { return candidate; }
        }
        return added;
    }

    namespace
    {
        bool paint_ferry(const Json& object)
        {
            if (!object.IsObject() || !object.HasMember("motion") || !object["motion"].IsObject()) { return false; }
            const Json& motion = object["motion"];
            return motion.HasMember("drive") && motion["drive"].IsString() &&
                std::string_view(motion["drive"].GetString()) == "paint";
        }

        // Names are labels, not shape: a centred object reflects onto itself
        // under another name. A ferry's heading and phase have no mirror rule.
        void strip_unmirrored(Json& object, bool ferry)
        {
            if (object.IsObject()) { object.RemoveMember("name"); }
            if (ferry)
            {
                Json& motion = object["motion"];
                for (const char* field : { "drive", "team_a_heads", "phase" }) { motion.RemoveMember(field); }
            }
        }

        // Equal shapes up to the rounding the reflection itself introduces.
        bool same_shape(const Json& a, const Json& b)
        {
            if (a.IsNumber() && b.IsNumber())
            {
                const double x = a.GetDouble(), y = b.GetDouble();
                return std::abs(x - y) <= 1e-6 * std::max({ 1.0, std::abs(x), std::abs(y) });
            }
            if (a.IsObject() && b.IsObject())
            {
                if (a.MemberCount() != b.MemberCount()) { return false; }
                for (auto it = a.MemberBegin(); it != a.MemberEnd(); ++it)
                {
                    const auto other = b.FindMember(it->name);
                    if (other == b.MemberEnd() || !same_shape(it->value, other->value)) { return false; }
                }
                return true;
            }
            if (a.IsArray() && b.IsArray())
            {
                if (a.Size() != b.Size()) { return false; }
                for (rapidjson::SizeType i = 0; i < a.Size(); ++i)
                {
                    if (!same_shape(a[i], b[i])) { return false; }
                }
                return true;
            }
            return a == b;
        }
    }

    namespace
    {
        // ED#27: the clipboard names the list an object came from, so a paste
        // lands in the same list of another document.
        constexpr std::array<std::pair<EntryKind, const char*>, 5> clipboard_lists = { {
            { EntryKind::collision, "collision_objects" }, { EntryKind::decoration, "non_collision_objects" },
            { EntryKind::spawn_a, "team_a" }, { EntryKind::spawn_b, "team_b" }, { EntryKind::station, "stations" } } };
        constexpr const char* clipboard_marker = "colourwars_level_object";
    }

    std::string LevelDocument::copy_text(const Entry& entry) const
    {
        const auto list = std::find_if(clipboard_lists.begin(), clipboard_lists.end(),
            [&](const auto& candidate) { return candidate.first == entry.kind; });
        if (list == clipboard_lists.end())
        { fail("Select an object, spawn or station to copy; level settings and bounds are not copied."); }
        // entry_json needs a mutable document; copy rather than cast away const.
        Document work;
        work.CopyFrom(impl_->document, work.GetAllocator());
        Document clip(rapidjson::kObjectType);
        clip.AddMember(rapidjson::StringRef(clipboard_marker), 1, clip.GetAllocator());
        clip.AddMember("list", rapidjson::StringRef(list->second), clip.GetAllocator());
        Json copied;
        copied.CopyFrom(entry_json(work, entry), clip.GetAllocator());
        clip.AddMember("object", copied, clip.GetAllocator());
        return json_text(clip);
    }

    Entry LevelDocument::paste_text(std::string_view text)
    {
        Document clip;
        try { clip = parse(text); }
        catch (const std::exception& error)
        { fail(std::string("The clipboard does not hold a copied level object. ") + error.what()); }
        if (!clip.IsObject() || !clip.HasMember(clipboard_marker) || !clip[clipboard_marker].IsInt() ||
            clip[clipboard_marker].GetInt() != 1 || !clip.HasMember("list") || !clip["list"].IsString() ||
            !clip.HasMember("object") || !clip["object"].IsObject())
        {
            fail("The clipboard does not hold a level object copied by this editor "
                "(it needs colourwars_level_object 1, a list and an object).");
        }
        const std::string list(clip["list"].GetString(), clip["list"].GetStringLength());
        const auto found = std::find_if(clipboard_lists.begin(), clipboard_lists.end(),
            [&](const auto& candidate) { return list == candidate.second; });
        if (found == clipboard_lists.end()) { fail("The clipboard names an unknown level list '" + list + "'."); }
        Entry added{ found->first, 0, {} };
        impl_->mutate([&](Document& document)
        {
            Allocator& allocator = document.GetAllocator();
            Json value;
            value.CopyFrom(clip["object"], allocator);
            if (added.kind == EntryKind::station && value.HasMember("name") && value["name"].IsString())
            {
                // A pasted station keeps its name and lesson unless that name
                // is taken here; then it is renamed as a duplicate would be
                // and its lesson stays with the station already named so.
                const std::string name = string(value, "name");
                if (unused_station_name(document, name, false) != name)
                {
                    const std::string renamed = unused_station_name(document, name + "_copy", false);
                    value["name"].SetString(renamed.data(), static_cast<rapidjson::SizeType>(renamed.size()), allocator);
                    value.RemoveMember("lesson");
                }
            }
            Json& objects = entry_array(document, added.kind);
            added.index = objects.Size();
            objects.PushBack(value, allocator);
        });
        for (const Entry& candidate : entries())
        {
            if (candidate.kind == added.kind && candidate.index == added.index) { return candidate; }
        }
        return added;
    }

    LevelDocument::SymmetryReport LevelDocument::symmetry() const
    {
        // Work on a copy, so the reflection helpers can edit freely.
        Document work;
        work.CopyFrom(impl_->document, work.GetAllocator());
        Allocator& allocator = work.GetAllocator();
        const Rect world = plain_rectangle(member(member(work, "level_bounds"), "out_of_bounds"));
        const double sum = world.x + (world.x + world.width);
        const double spawn_width = impl_->motion_tuning.empty() ? 52.0 :
            static_cast<double>(load_motion_player_size(impl_->motion_tuning).width);
        const std::vector<Entry> all = entries();
        const auto listed = [&](EntryKind kind)
        {
            std::vector<Entry> result;
            for (const Entry& entry : all) { if (entry.kind == kind) { result.push_back(entry); } }
            return result;
        };
        SymmetryReport report;

        // Spawns pair across teams, one to one, in document order.
        {
            const std::vector<Entry> team_a = listed(EntryKind::spawn_a), team_b = listed(EntryKind::spawn_b);
            std::vector<bool> used(team_b.size(), false);
            for (const Entry& spawn : team_a)
            {
                Json reflected;
                reflected.CopyFrom(entry_json(work, spawn), allocator);
                set_number(reflected, "x", sum - number(reflected, "x") - spawn_width);
                bool found = false;
                for (std::size_t j = 0; j < team_b.size() && !found; ++j)
                {
                    if (used[j] || !same_shape(reflected, entry_json(work, team_b[j]))) { continue; }
                    used[j] = found = true;
                }
                if (!found) { report.unmatched.push_back(spawn); }
            }
            for (std::size_t j = 0; j < team_b.size(); ++j)
            {
                if (!used[j]) { report.unmatched.push_back(team_b[j]); }
            }
        }

        // Objects pair within their own list; a self-reflection is its own partner.
        for (const EntryKind kind : { EntryKind::collision, EntryKind::decoration })
        {
            const std::vector<Entry> objects = listed(kind);
            std::vector<Json> shapes(objects.size()), reflections(objects.size());
            std::vector<bool> ferry(objects.size(), false), mirrored(objects.size(), false);
            for (std::size_t i = 0; i < objects.size(); ++i)
            {
                const Json& source = entry_json(work, objects[i]);
                ferry[i] = paint_ferry(source);
                shapes[i].CopyFrom(source, allocator);
                strip_unmirrored(shapes[i], ferry[i]);
                // Reflect the path alone: without its drive a ferry mirrors as
                // an ordinary mover, and its phase is then dropped like its heading.
                reflections[i].CopyFrom(source, allocator);
                if (ferry[i])
                {
                    for (const char* field : { "drive", "team_a_heads" }) { reflections[i]["motion"].RemoveMember(field); }
                }
                // A malformed record has no reflection; it simply stays unmatched.
                try { mirror_object(reflections[i], sum, allocator); mirrored[i] = true; }
                catch (const std::exception&) {}
                if (mirrored[i])
                {
                    if (ferry[i]) { reflections[i]["motion"].RemoveMember("phase"); }
                    reflections[i].RemoveMember("name");
                }
            }
            std::vector<bool> used(objects.size(), false);
            for (std::size_t i = 0; i < objects.size(); ++i)
            {
                if (used[i]) { continue; }
                if (mirrored[i])
                {
                    for (std::size_t j = i; j < objects.size(); ++j)
                    {
                        if (used[j] || ferry[j] != ferry[i] || !same_shape(reflections[i], shapes[j])) { continue; }
                        used[i] = used[j] = true;
                        if (ferry[i])
                        {
                            report.paint_ferries.push_back(objects[i]);
                            if (j != i) { report.paint_ferries.push_back(objects[j]); }
                        }
                        break;
                    }
                }
                if (!used[i]) { (kind == EntryKind::decoration ? report.unmatched_art : report.unmatched).push_back(objects[i]); }
            }
        }
        return report;
    }

    void LevelDocument::erase(const Entry& entry)
    {
        impl_->mutate([&](Document& document)
        {
            entry_json(document, entry);
            Json& objects = entry_array(document, entry.kind);
            objects.Erase(objects.Begin() + static_cast<std::ptrdiff_t>(entry.index));
            // The array is optional; deleting the last station leaves none.
            if (entry.kind == EntryKind::station && objects.Empty()) { document.RemoveMember("stations"); }
        });
    }

    bool LevelDocument::undo()
    {
        if (!can_undo()) { return false; }
        Document previous = parse(impl_->history[impl_->history_index - 1]);
        validate_document(previous, impl_->motion_tuning);
        impl_->document.Swap(previous);
        --impl_->history_index;
        return true;
    }

    bool LevelDocument::redo()
    {
        if (!can_redo()) { return false; }
        Document next = parse(impl_->history[impl_->history_index + 1]);
        validate_document(next, impl_->motion_tuning);
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
        try { validate_document(impl_->document, impl_->motion_tuning); }
        catch (const std::exception& error) { return { error.what() }; }
        return {};
    }

    std::vector<std::string> LevelDocument::unresolved_resources(const ContentCatalog& catalog) const
    {
        // The names LevelLoadedInfo::validate_content and LevelObjectBuilder resolve
        // at startup. A file the catalog could not read leaves its names unchecked.
        std::vector<std::string> result;
        if (!catalog.manifest_read) { return result; }
        const auto sheet_of = [&](const std::string& sheet) { return catalog.sprite_frames.find(sheet); };
        try
        {
            const Document& document = impl_->document;
            // Match LevelLoadedInfo's legacy icon_frame validation. Stage Select
            // draws a live preview, but the metadata must still name a real frame.
            const std::string icon = string(document, "icon_frame");
            const auto icons = sheet_of("sprite_sheet_1");
            if (icons == catalog.sprite_frames.end())
            {
                result.push_back("icon_frame: manifest.json has no sprite sheet sprite_sheet_1.");
            }
            else if (icons->second && !icons->second->contains(icon))
            {
                result.push_back("icon_frame '" + icon + "' is not a frame in sprite_sheet_1.");
            }
            const Json& sound = member(document, "sound");
            const std::string bank_name = string(sound, "sound_bank_name");
            const auto bank = catalog.sound_effects.find(bank_name);
            if (bank == catalog.sound_effects.end())
            {
                result.push_back("sound_bank_name '" + bank_name + "' is not a sound bank in manifest.json.");
            }
            else if (bank->second)
            {
                for (const char* field : { "music_name", "end_sting_name" })
                {
                    if (!sound.HasMember(field)) { continue; }
                    const std::string effect = string(sound, field);
                    if (!bank->second->contains(effect))
                    {
                        result.push_back(std::string(field) + " '" + effect +
                            "' is not an effect in sound bank " + bank_name + ".");
                    }
                }
            }
            for (const char* list : { "collision_objects", "non_collision_objects" })
            {
                const Json& objects = array(document, list);
                for (rapidjson::SizeType index = 0; index < objects.Size(); ++index)
                {
                    const Json& object = objects[index];
                    const std::string where = object_label(object, index) + ": ";
                    if (string(object, "type") == "FixedSlabPaintable")
                    {
                        const auto terrain = sheet_of("terrain");
                        if (terrain == catalog.sprite_frames.end())
                            result.push_back(where + "fixed slab material needs sprite sheet terrain.");
                        else if (terrain->second && !terrain->second->contains("swatch_eraser_fill"))
                            result.push_back(where + "fixed slab material needs terrain frame swatch_eraser_fill.");
                        continue;
                    }
                    const std::string sheet = string(object, "sheet_name");
                    const std::string frame = string(object, "frame_name");
                    const auto frames = sheet_of(sheet);
                    if (frames == catalog.sprite_frames.end())
                    {
                        result.push_back(where + "sheet_name '" + sheet + "' is not a sprite sheet in manifest.json.");
                    }
                    else if (frames->second && !frames->second->contains(frame))
                    {
                        result.push_back(where + "frame_name '" + frame + "' is not a frame in sprite sheet " +
                            sheet + ".");
                    }
                }
            }
        }
        // A structural error is validate()'s to report, not a resource name.
        catch (const std::exception&) { return result; }
        return result;
    }
}
