#pragma once

// Included after the existing MechanismBitmap helper. Exercises the actual
// inspector/canvas in --gestures, apart from the unactivated DPI/GDI fixture.
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>
#include <iterator>

namespace colourwars_editor
{
    namespace
    {
        std::string empty_key_native_bytes(const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Could not read the empty-key native private file");
            const std::string bytes{ std::istreambuf_iterator<char>(input), {} };
            require(!input.bad(), "Could not read the complete empty-key native private file");
            return bytes;
        }

        rapidjson::Document empty_key_native_parse(const std::string& text)
        {
            rapidjson::Document parsed;
            parsed.Parse(text.data(), text.size());
            require(!parsed.HasParseError() && parsed.IsObject(), "Empty-key native fixture is not a JSON object");
            return parsed;
        }

        std::string empty_key_native_json(const rapidjson::Value& value)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            require(value.Accept(writer), "Could not serialize the empty-key native fixture");
            return std::string(buffer.GetString(), buffer.GetSize()) + '\n';
        }
    }

    inline void EditorDpiProbe::check_empty_key_properties(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto original_path = editor.document_.path();
        const auto original_selection = editor.selection_;
        const double original_zoom = editor.zoom_, original_x = editor.view_x_, original_y = editor.view_y_;
        const HWND original_focus = GetFocus();
        const auto stage = scratch / "empty-key-inspector.json";
        LevelDocument seed;
        const Entry object = seed.add_object("StructurePaintable", { 100, 100, 160, 160 });
        const Entry level{ EntryKind::level, 0, "Level settings" };
        rapidjson::Document source = empty_key_native_parse(seed.serialize());
        auto& allocator = source.GetAllocator();
        const std::string stage_name = source["display_name"].GetString();
        auto& collision = source["collision_objects"][static_cast<rapidjson::SizeType>(object.index)];
        const std::string object_name = collision["name"].GetString();
        rapidjson::Value level_metadata(rapidjson::kObjectType);
        level_metadata.AddMember("display_name", "ED16 nested level name", allocator);
        source.AddMember("", level_metadata, allocator);
        rapidjson::Value object_metadata(rapidjson::kObjectType);
        object_metadata.AddMember("type", "StructureRamp", allocator);
        object_metadata.AddMember("collision_type", "STRUCTURE_RAMP_RIGHT", allocator);
        object_metadata.AddMember("name", "ED16 nested object name", allocator);
        collision.AddMember("", object_metadata, allocator);
        const std::string disk = empty_key_native_json(source);
        {
            std::ofstream output(stage, std::ios::binary | std::ios::trunc);
            output.write(disk.data(), static_cast<std::streamsize>(disk.size()));
            require(output.good(), "Could not write the empty-key native private file");
        }
        editor.document_.load(stage);
        editor.selection_ = level;
        editor.refresh();
        const std::string baseline = editor.document_.serialize();
        require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
            !editor.document_.can_redo() && !IsWindowVisible(editor.window_),
            "Empty-key native fixture did not begin at its clean hidden baseline");
        const auto row_for_value = [&](const std::string& value)
        {
            int row = -1;
            for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            {
                if (editor.properties_[index].value != value) { continue; }
                require(row < 0, "Empty-key native fixture value is not unique");
                row = static_cast<int>(index);
            }
            require(row >= 0, "Empty-key native inspector lost its metadata value");
            return row;
        };
        const auto edit_value = [&](const std::string& previous, const wchar_t* replacement)
        {
            const int row = row_for_value(previous);
            ListView_SetItemState(editor.property_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_SetItemState(editor.property_list_, row, LVIS_SELECTED | LVIS_FOCUSED,
                LVIS_SELECTED | LVIS_FOCUSED);
            require(editor.property_index_ == row &&
                ListView_GetNextItem(editor.property_list_, -1, LVNI_SELECTED) == row &&
                IsWindowEnabled(editor.apply_) && !editor.pending(),
                "Empty-key native metadata row was not selected for ordinary editing");
            SetWindowTextW(editor.value_edit_, replacement);
            require(editor.pending(), "Empty-key native edit did not reach the inspector text control");
            SendMessageW(editor.window_, WM_COMMAND, MAKEWPARAM(apply_property, BN_CLICKED),
                reinterpret_cast<LPARAM>(editor.apply_));
            require(!editor.pending(), "Empty-key native Apply left its text pending");
        };

        // First new regression uses the real row chosen by its unique value,
        // not a hand-written pointer that would bypass the faulty generator.
        edit_value("ED16 nested level name", L"ED16 edited level metadata");
        auto parsed = empty_key_native_parse(editor.document_.serialize());
        require(std::string(parsed["display_name"].GetString()) == stage_name &&
            std::string(parsed[""]["display_name"].GetString()) == "ED16 edited level metadata",
            "Empty-key inspector edit changed the real level field instead of its metadata leaf");
        require(editor.properties_[static_cast<std::size_t>(row_for_value("ED16 edited level metadata"))].path ==
            "//display_name", "Empty-key inspector row lost its explicit empty first pointer token");
        const std::string first = editor.document_.serialize();
        require(editor.document_.dirty() && editor.document_.can_undo() && !editor.document_.can_redo() &&
            editor.document_.path() == stage && empty_key_native_bytes(stage) == disk,
            "Empty-key native Apply changed the path/source or lost its first history entry");

        editor.select(object);
        edit_value("ED16 nested object name", L"ED16 edited object metadata");
        parsed = empty_key_native_parse(editor.document_.serialize());
        const auto& actual = parsed["collision_objects"][static_cast<rapidjson::SizeType>(object.index)];
        require(std::string(actual["name"].GetString()) == object_name &&
            std::string(actual[""]["name"].GetString()) == "ED16 edited object metadata" &&
            std::string(parsed["display_name"].GetString()) == stage_name &&
            std::string(parsed[""]["display_name"].GetString()) == "ED16 edited level metadata",
            "Empty-key native object Apply changed a real object field or another metadata target");
        const std::string second = editor.document_.serialize();

        const auto draw = std::find_if(editor.draw_items_.begin(), editor.draw_items_.end(),
            [&](const DrawItem& item) { return same_entry(item.entry, object); });
        require(draw != editor.draw_items_.end() && draw->values.at("type") == "StructurePaintable" &&
            draw->values.at("collision_type") == "STRUCTURE_PAINTABLE" &&
            draw->values.at("/type") == "StructureRamp" &&
            draw->values.at("/collision_type") == "STRUCTURE_RAMP_RIGHT",
            "Empty-key metadata shadowed the actual object's native preview type");
        editor.zoom_ = 1; editor.view_x_ = editor.view_y_ = 0;
        MechanismBitmap bitmap(512, 512);
        const auto selected = editor.selection_;
        editor.selection_.reset();
        editor.draw_item(bitmap.dc(), *draw);
        editor.selection_ = selected;
        GdiFlush();
        require(bitmap.pixel(125, 125) == MechanismBitmap::colour(RGB(114, 133, 149)) &&
            bitmap.pixel(235, 235) == MechanismBitmap::colour(RGB(114, 133, 149)),
            "Empty-key metadata turned the rectangular native body into a ramp");
        for (POINT point : { POINT{ 125, 125 }, POINT{ 235, 235 } })
        {
            const auto hit = editor.hit(point);
            require(hit && same_entry(*hit, object),
                "Empty-key metadata changed the rectangular native object's hit shape");
        }
        bitmap.save(scratch / "empty-key-object-body.bmp");
        require(editor.document_.serialize() == second && editor.document_.path() == stage &&
            empty_key_native_bytes(stage) == disk && editor.document_.dirty() &&
            editor.document_.can_undo() && !editor.document_.can_redo(),
            "Empty-key native drawing/query changed the source or complete edit state");

        const auto undo = [&] { SendMessageW(editor.window_, WM_COMMAND, edit_undo, 0); };
        const auto redo = [&] { SendMessageW(editor.window_, WM_COMMAND, edit_redo, 0); };
        undo();
        require(editor.document_.serialize() == first && editor.document_.can_undo() && editor.document_.can_redo(),
            "Empty-key native history did not undo the object metadata alone");
        undo();
        require(editor.document_.serialize() == baseline && !editor.document_.dirty() &&
            !editor.document_.can_undo() && editor.document_.can_redo(),
            "Empty-key native history lost its exact saved baseline or gained an edit");
        redo();
        require(editor.document_.serialize() == first, "Empty-key native history lost its first redo state");
        redo();
        require(editor.document_.serialize() == second && !editor.document_.can_redo() &&
            editor.save(false) && empty_key_native_bytes(stage) == second && !editor.document_.dirty(),
            "Empty-key native Save did not retain its exact metadata targets");
        LevelDocument reopened;
        reopened.load(stage);
        require(reopened.serialize() == second, "Empty-key native Save/reopen lost metadata bytes");
        undo();
        require(editor.document_.serialize() == first && editor.document_.dirty(),
            "Empty-key native Save lost its earlier unsaved history state");
        redo();
        require(editor.document_.serialize() == second && !editor.document_.dirty() &&
            !editor.document_.can_redo() && editor.selection_ && same_entry(*editor.selection_, object) &&
            editor.document_.path() == stage && !IsWindowVisible(editor.window_),
            "Empty-key native saved redo state lost its selection/path/hidden boundary");

        editor.document_.load(original_path);
        editor.selection_ = original_selection;
        editor.refresh();
        editor.zoom_ = original_zoom; editor.view_x_ = original_x; editor.view_y_ = original_y;
        SetFocus(original_focus);
        require(GetFocus() == original_focus && !IsWindowVisible(editor.window_),
            "Empty-key native inspector did not restore its hidden caller shell and focus");
        std::cout << "PASS native empty-key metadata: actual inspector exact targets, known type shadow control, "
            "rectangular body pixels/hits, complete two-edit history and private Save/reopen\n";
    }
}
