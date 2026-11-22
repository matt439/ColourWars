#pragma once

#include "scripts/acceptance/level_editor_dpi/pending_modal_driver.h"

#include <ShlObj_core.h>

#include <cstring>
#include <fstream>
#include <initializer_list>

namespace colourwars_editor
{
    namespace
    {
        void send_file_drop(HWND window, std::initializer_list<std::filesystem::path> paths)
        {
            std::wstring filenames;
            for (const auto& path : paths)
            {
                filenames += path.native();
                filenames.push_back(L'\0');
            }
            const std::size_t bytes = (filenames.size() + 1) * sizeof(wchar_t);
            HGLOBAL memory = GlobalAlloc(GHND, sizeof(DROPFILES) + bytes);
            require(memory != nullptr, "Could not allocate the private file drop");
            auto* payload = static_cast<DROPFILES*>(GlobalLock(memory));
            if (!payload)
            {
                GlobalFree(memory);
                throw std::runtime_error("Could not lock the private file drop");
            }
            payload->pFiles = static_cast<DWORD>(sizeof(DROPFILES));
            payload->fWide = TRUE;
            std::memcpy(payload + 1, filenames.c_str(), bytes);
            GlobalUnlock(memory);
            // As with a Shell drop, the receiving window owns the handle.
            SendMessageW(window, WM_DROPFILES, reinterpret_cast<WPARAM>(memory), 0);
        }
    }

    inline void EditorDpiProbe::check_file_drop(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto source = scratch / "drop-source.json";
        const auto target = scratch / L"drop target \u03bb.JSON";
        const auto unrelated = scratch / "drop-not-a-level.txt";
        const auto directory = scratch / "drop-directory.json";
        const auto invalid = scratch / "drop-invalid.json";
        const Entry spawn{ EntryKind::spawn_a, 0, "Team A spawn 1" };
        LevelDocument seed;
        seed.save(source);
        const double start = seed.rectangle(spawn).x;
        seed.set_property(spawn, "x", std::to_string(start + 100.0));
        seed.save(target);
        seed.save(unrelated);
        std::filesystem::create_directories(directory);
        std::ofstream(invalid, std::ios::binary | std::ios::trunc) << "{";
        std::wstring diagnostic;
        try { seed.load(invalid); }
        catch (const std::exception& exception) { diagnostic = wide(exception.what()); }
        require(!diagnostic.empty(), "The private invalid drop fixture did not produce a diagnostic");

        require((GetWindowLongPtrW(editor.window_, GWL_EXSTYLE) & WS_EX_ACCEPTFILES) != 0,
            "The editor window did not register for file drops");
        editor.document_.load(source);
        editor.reset_document_view();
        send_file_drop(editor.window_, { target, source });
        send_file_drop(editor.window_, { unrelated });
        send_file_drop(editor.window_, { directory });
        require(editor.document_.path() == source && !editor.document_.dirty() &&
            window_text(editor.status_) == L"Drop one level JSON file to open it.",
            "A multiple, unrelated or directory drop replaced the document");

        editor.document_.set_property(spawn, "x", std::to_string(start + 25.0));
        editor.document_.set_property(spawn, "x", std::to_string(start + 50.0));
        require(editor.document_.undo(), "Could not prepare redo for the file drop");
        editor.select(spawn);
        const auto property = std::find_if(editor.properties_.begin(), editor.properties_.end(),
            [](const Property& value) { return value.path == "x"; });
        require(property != editor.properties_.end(), "The drop fixture needs a spawn X property");
        editor.select_property(static_cast<int>(property - editor.properties_.begin()));
        const std::wstring pending = std::to_wstring(start + 75.0);
        SetWindowTextW(editor.value_edit_, pending.c_str());
        editor.zoom_ *= 1.5;
        const double zoom = editor.zoom_;
        const auto current = editor.document_.serialize();
        const auto unchanged = [&]
        {
            require(editor.document_.path() == source && editor.document_.serialize() == current &&
                editor.document_.dirty() && editor.document_.can_undo() && editor.document_.can_redo() &&
                editor.selection_ && same_entry(*editor.selection_, spawn) && editor.pending() &&
                window_text(editor.value_edit_) == pending && editor.zoom_ == zoom,
                "A cancelled or failed drop changed the open level, edits, selection or view");
        };
        const wchar_t* question = L"Save your changes before continuing?";
        PendingDialogs dialogs(editor.window_);
        dialogs.begin({ { L"Unsaved level", question, IDCANCEL } });
        send_file_drop(editor.window_, { target });
        require(dialogs.finish(), "The file drop did not offer to keep the open edits");
        unchanged();

        dialogs.begin({ { L"Unsaved level", question, IDNO },
            { L"ColourWars Level Editor", diagnostic.c_str(), IDOK, true } });
        send_file_drop(editor.window_, { invalid });
        require(dialogs.finish(), "The file drop did not report an invalid level through File Open's error path");
        unchanged();

        dialogs.begin({ { L"Unsaved level", question, IDYES } });
        send_file_drop(editor.window_, { target });
        require(dialogs.finish() && dialogs.finish_fixture(), "The file drop did not save the open edits");
        require(editor.document_.path() == target && !editor.document_.dirty() && !editor.pending() &&
            !editor.document_.can_undo() && !editor.document_.can_redo() && editor.group_.empty() &&
            editor.selection_ && editor.selection_->kind == EntryKind::level && editor.zoom_ != zoom &&
            editor.document_.rectangle(spawn).x == start + 100.0,
            "The file drop did not open and fit the Unicode target as a fresh document");
        seed.load(source);
        require(seed.rectangle(spawn).x == start + 75.0, "The file drop did not save the pending property edit");
        std::cout << "PASS editor file drop: single Unicode JSON, invalid drop filters, Cancel, malformed level and Save\n";
    }
}
