#pragma once

// Included once after EditorDpiProbe. Its existing friendship exercises the
// real inspector and Save path without exposing a production test API.
#include <fstream>
#include <iterator>

namespace colourwars_editor
{
    namespace
    {
        std::string metadata_bytes(const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Could not read the native metadata scratch file");
            return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
        }

        void write_metadata(const std::filesystem::path& path, const std::string& text)
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(text.data(), static_cast<std::streamsize>(text.size()));
            require(output.good(), "Could not write the native metadata scratch file");
        }
    }

    inline void EditorDpiProbe::check_metadata(Editor& editor, const std::filesystem::path& scratch)
    {
        const std::filesystem::path original_path = editor.document_.path();
        const std::filesystem::path stage = scratch / "nul-metadata.json";
        const Entry level{ EntryKind::level, 0, "Level settings" };
        const Entry spawn{ EntryKind::spawn_a, 0, "Team A spawn 1" };
        std::string source = editor.document_.serialize();
        source.insert(source.find('{') + 1, R"(
            "note":"keep\u0000suffix",
            "literal_note":"keep\\u0000suffix",
            "mixed_note":"literal\\u0000\u0000tail",
            "key":"prefix sibling",
            "key\u0000tail":"literal\\u0000",
            "numeric":7,
            "numeric\u0000tail":42,
            "flag":false,
            "flag\u0000tail":true,
            "nested\u0000scope":{"child":"nested value"},
        )");
        const std::size_t floor_name = source.find("\"floor\"");
        require(floor_name != std::string::npos, "Native metadata fixture has no floor name");
        source.replace(floor_name, std::string("\"floor\"").size(), R"("floor\u0000tail")");
        write_metadata(stage, source);
        editor.document_.load(stage);
        editor.selection_.reset();
        editor.refresh();
        const std::string original = editor.document_.serialize();
        require(!editor.properties_.empty() && editor.property_index_ == 0 &&
            editor.properties_.front().path == "note", "Native metadata fixture must initially show its NUL value");

        // Save is the first operation under test: the old inspector silently
        // applied its truncated control value here, before any user edit.
        require(editor.save(false), "Untouched native metadata Save did not complete");
        require(metadata_bytes(stage) == original,
            "Untouched native Save truncated NUL-bearing metadata bytes");
        LevelDocument reopened;
        reopened.load(stage);
        require(reopened.serialize() == original,
            "Untouched native Save/reopen changed NUL-bearing metadata");
        const auto reopened_properties = reopened.properties(level);
        require(!reopened_properties.empty() && reopened_properties.front().value ==
            std::string("keep") + '\0' + "suffix",
            "Untouched native Save/reopen lost the full metadata string length");
        require(!editor.document_.dirty() && !editor.document_.can_undo() && !editor.document_.can_redo(),
            "Untouched native Save manufactured an edit or history entry");

        const auto row_for = [&](const std::string& path)
        {
            for (std::size_t index = 0; index < editor.properties_.size(); ++index)
                if (editor.properties_[index].path == path) { return static_cast<int>(index); }
            throw std::runtime_error("Native metadata fixture property is missing");
        };
        const auto choose = [&](const std::string& path)
        {
            const int row = row_for(path);
            ListView_SetItemState(editor.property_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_SetItemState(editor.property_list_, row, LVIS_SELECTED | LVIS_FOCUSED,
                LVIS_SELECTED | LVIS_FOCUSED);
            require(editor.property_index_ == row &&
                ListView_GetNextItem(editor.property_list_, -1, LVNI_SELECTED) == row,
                "Native metadata selection was blocked or chose another property");
            return row;
        };
        const auto list_text = [&](int row, int column)
        {
            std::array<wchar_t, 1024> text{};
            ListView_GetItemText(editor.property_list_, row, column, text.data(), static_cast<int>(text.size()));
            return std::wstring(text.data());
        };
        const auto readonly = [&](const std::string& path, const std::wstring& label,
            const std::wstring& value, bool numeric = false, bool boolean = false)
        {
            const int row = choose(path);
            const Property& property = editor.properties_[static_cast<std::size_t>(row)];
            require(property.numeric == numeric && property.boolean == boolean,
                "Native metadata preview changed its underlying primitive type");
            require(IsWindowEnabled(editor.value_edit_) &&
                (GetWindowLongPtrW(editor.value_edit_, GWL_STYLE) & ES_READONLY) != 0 &&
                !IsWindowEnabled(editor.apply_) && !IsWindowEnabled(editor.revert_),
                "NUL-bearing metadata did not become a selectable read-only preview");
            require(!editor.pending() && window_text(editor.value_edit_) == value &&
                list_text(row, 0) == label && list_text(row, 1) == value,
                "NUL-bearing metadata lost its escaped label/value preview or became pending");
            const std::wstring explanation = window_text(editor.property_label_);
            require(explanation.starts_with(L"Read-only") && explanation.find(label) != std::wstring::npos,
                "The metadata inspector did not explain read-only status before its full escaped label");
        };
        const std::string nul_key = std::string("key") + '\0' + "tail";
        readonly("note", L"\"note\"", L"\"keep\\u0000suffix\"");
        readonly("mixed_note", L"\"mixed_note\"", L"\"literal\\\\u0000\\u0000tail\"");
        readonly(nul_key, L"\"key\\u0000tail\"", L"\"literal\\\\u0000\"");
        readonly(std::string("numeric") + '\0' + "tail", L"\"numeric\\u0000tail\"", L"42", true);
        readonly(std::string("flag") + '\0' + "tail", L"\"flag\\u0000tail\"", L"true", false, true);
        readonly(std::string("nested") + '\0' + "scope/child",
            L"\"nested\\u0000scope/child\"", L"\"nested value\"");

        bool found_floor = false;
        for (std::size_t index = 0; index < editor.entries_.size(); ++index)
        {
            if (editor.entries_[index].kind != EntryKind::collision || editor.entries_[index].index != 0) { continue; }
            std::array<wchar_t, 1024> label{};
            require(SendMessageW(editor.object_list_, LB_GETTEXT, index,
                reinterpret_cast<LPARAM>(label.data())) != LB_ERR, "Could not read the native object's metadata label");
            require(std::wstring(label.data()) == L"\"floor\\u0000tail\"",
                "The native object list truncated its NUL-bearing name");
            found_floor = true;
        }
        require(found_floor, "Native metadata fixture floor disappeared");

        // Even a programmatic WM_SETTEXT must not turn a read-only preview into
        // model data via Apply/Enter; Revert simply restores its escaped text.
        readonly("note", L"\"note\"", L"\"keep\\u0000suffix\"");
        SetWindowTextW(editor.value_edit_, L"must not replace preserved metadata");
        require(!editor.pending(), "A read-only metadata preview was treated as pending");
        editor.command(apply_property);
        editor.command(revert_property);
        require(editor.document_.serialize() == original && !editor.document_.dirty() &&
            !editor.document_.can_undo() && !editor.document_.can_redo() &&
            window_text(editor.value_edit_) == L"\"keep\\u0000suffix\"",
            "Applying a read-only metadata preview changed data or history");

        readonly(nul_key, L"\"key\\u0000tail\"", L"\"literal\\\\u0000\"");
        editor.refresh(true);
        require(editor.properties_[static_cast<std::size_t>(editor.property_index_)].path == nul_key &&
            editor.document_.serialize() == original && !editor.pending(),
            "Native refresh lost the complete NUL property path or changed its data");
        editor.select(spawn);
        editor.select(level);
        require(editor.document_.serialize() == original && !editor.document_.dirty() &&
            !editor.document_.can_undo() && !editor.document_.can_redo(),
            "Native object selection applied truncated metadata or changed history");

        choose("literal_note");
        require((GetWindowLongPtrW(editor.value_edit_, GWL_STYLE) & ES_READONLY) == 0 &&
            IsWindowEnabled(editor.value_edit_) && IsWindowEnabled(editor.apply_) && IsWindowEnabled(editor.revert_) &&
            window_text(editor.value_edit_) == L"keep\\u0000suffix",
            "Ordinary literal-backslash text did not restore plaintext editing");
        SendMessageW(editor.value_edit_, EM_SETSEL, 0, -1);
        SendMessageW(editor.value_edit_, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L"ordinary \\u0000 text"));
        require(editor.pending() && editor.apply_pending(), "Ordinary plaintext editing did not apply");
        const std::string first_edit = editor.document_.serialize();
        require(editor.properties_[static_cast<std::size_t>(row_for("literal_note"))].value == "ordinary \\u0000 text" &&
            editor.document_.dirty() && editor.document_.can_undo() && !editor.document_.can_redo(),
            "Ordinary plaintext editing decoded escapes or lost its history");
        choose("numeric");
        require(window_text(editor.value_edit_) == L"7" &&
            (GetWindowLongPtrW(editor.value_edit_, GWL_STYLE) & ES_READONLY) == 0 &&
            IsWindowEnabled(editor.apply_) && IsWindowEnabled(editor.revert_),
            "Ordinary numeric editing retained read-only preview state");
        SetWindowTextW(editor.value_edit_, L"9");
        require(editor.pending() && editor.apply_pending(), "Ordinary numeric editing did not apply");
        const std::string second_edit = editor.document_.serialize();
        editor.command(edit_undo);
        require(editor.document_.serialize() == first_edit && editor.document_.can_undo() &&
            editor.document_.can_redo() && editor.document_.dirty(),
            "Could not prepare the metadata inspector's full undo/redo branch");

        readonly(nul_key, L"\"key\\u0000tail\"", L"\"literal\\\\u0000\"");
        editor.refresh(true);
        editor.select(spawn);
        editor.select(level);
        readonly(nul_key, L"\"key\\u0000tail\"", L"\"literal\\\\u0000\"");
        require(editor.document_.serialize() == first_edit && editor.document_.can_undo() &&
            editor.document_.can_redo() && editor.document_.dirty(),
            "Metadata selection/refresh changed the prepared undo/redo branch");
        require(editor.save(false) && metadata_bytes(stage) == first_edit &&
            !editor.document_.dirty() && editor.document_.can_undo() && editor.document_.can_redo(),
            "Native metadata Save changed bytes or discarded the existing history branch");
        readonly(nul_key, L"\"key\\u0000tail\"", L"\"literal\\\\u0000\"");
        editor.command(edit_undo);
        require(editor.document_.serialize() == original && editor.document_.dirty(),
            "Metadata preview or Save changed the complete undo state");
        editor.command(edit_redo);
        require(editor.document_.serialize() == first_edit && !editor.document_.dirty(),
            "Metadata preview or Save changed the saved redo state");
        editor.command(edit_redo);
        require(editor.document_.serialize() == second_edit && editor.document_.dirty(),
            "Metadata preview or Save discarded the pending redo state");
        editor.command(edit_undo);
        reopened.load(stage);
        require(editor.document_.serialize() == first_edit && reopened.serialize() == first_edit &&
            !editor.document_.dirty() && editor.document_.can_undo() && editor.document_.can_redo(),
            "Native metadata round-trip lost bytes, saved state or complete history");
        const std::string resource = std::string("stage_close_quarters") + '\0' + "missing_suffix";
        editor.document_.set_property(level, "icon_frame", resource);
        editor.refresh(true);
        require(!editor.unresolved_.empty() && editor.unresolved_.front().find(resource) != std::string::npos,
            "Native resource warning discarded the complete raw NUL-bearing identifier");
        const std::wstring warning = window_text(editor.status_);
        require(warning.find(L"stage_close_quarters\\u0000missing_suffix") != std::wstring::npos &&
            warning.find(L"is not a frame in sprite_sheet_1.") != std::wstring::npos,
            "Native resource status truncated its escaped identifier suffix or refusal reason");
        require(!IsWindowVisible(editor.window_), "Native metadata checks showed the hidden editor");
        std::cout << "PASS native metadata: untouched Save full NUL bytes, typed escaped read-only previews, "
            "literal backslashes, selection/refresh, full history, restored plaintext editing and resource warnings\n";

        editor.document_.load(original_path);
        editor.selection_.reset();
        editor.refresh();
    }
}
