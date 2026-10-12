#include "editor/level_document.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "editor/level_json.h"
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
                "sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"GAINSBORO",
                "rectangle":{"x":0,"y":1200,"width":3000,"height":100},
                "collision_type":"STRUCTURE_PAINTABLE",
                "paintable_faces":{"left":false,"top":true,"right":false,"bottom":false}}],
            "non_collision_objects":[{"name":"background","type":"Visual",
                "sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"404040",
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
