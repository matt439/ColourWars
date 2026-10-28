#include "editor/editor.h"
#include "editor/win32_text.h"
#include "editor/user_stage_save.h"
#include "editor/save_guidance.h"
#include "editor/window_dpi.h"
#include "editor/window_layout.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>

namespace colourwars_editor
{
    namespace
    {
        std::wstring destination_guidance(const std::filesystem::path& path,
            const ContentCatalog& catalog, bool saved)
        {
            std::optional<std::filesystem::path> my_stages;
            try { my_stages = user_stage_folder(); }
            catch (const std::runtime_error&) {}
            return save_guidance(path, std::filesystem::path(COLOURWARS_CONTENT_DIR) / L"levels",
                catalog, my_stages, saved);
        }

        HFONT editor_font(UINT dpi)
        {
            HFONT font = CreateFontW(-editor_pixels(16, dpi), 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            if (!font) { throw std::runtime_error("Could not create the editor font."); }
            return font;
        }

        bool editor_file_dialog(OPENFILENAMEW& dialog, bool save)
        {
            // The legacy dialog can retain its selected directory despite
            // OFN_NOCHANGEDIR. Release that current-directory handle before
            // returning a path or cleaning up a cancelled My Stages folder.
            const std::filesystem::path original = std::filesystem::current_path();
            const BOOL accepted = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
            const DWORD error = accepted ? 0 : CommDlgExtendedError();
            std::filesystem::current_path(original);
            if (!accepted && error != 0) { throw std::runtime_error("The file dialog could not be opened."); }
            return accepted != FALSE;
        }
    }

    Editor::Editor(HINSTANCE instance) : instance_(instance),
        document_(std::filesystem::path(COLOURWARS_CONTENT_DIR) / L"tuning.json") {}
    Editor::~Editor()
    {
        if (window_ && IsWindow(window_)) { DestroyWindow(window_); }
        if (menu_ && IsMenu(menu_)) { DestroyMenu(menu_); }
        if (accelerators_) { DestroyAcceleratorTable(accelerators_); }
        if (font_) { DeleteObject(font_); }
    }
    int Editor::pixels(int logical) const { return editor_pixels(logical, dpi_); }
    HWND Editor::control(const wchar_t* type, const wchar_t* text, DWORD style, int id)
    {
        HWND child = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style,
            0, 0, 1, 1, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
        if (!child) throw std::runtime_error("Could not create an editor control.");
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        return child;
    }

    void Editor::create_menu()
    {
        menu_ = CreateMenu();
        HMENU file = CreatePopupMenu();
        AppendMenuW(file, MF_STRING, file_new, L"&New\tCtrl+N");
        AppendMenuW(file, MF_STRING, file_open, L"&Open...\tCtrl+O");
        AppendMenuW(file, MF_STRING, file_save, L"&Save\tCtrl+S");
        AppendMenuW(file, MF_STRING, file_save_as, L"Save &as...\tCtrl+Shift+S");
        AppendMenuW(file, MF_STRING, file_save_user_stage, L"Save to &My Stages...\tCtrl+M");
        AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(file, MF_STRING, file_exit, L"E&xit");
        AppendMenuW(menu_, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"&File");
        HMENU edit = CreatePopupMenu();
        AppendMenuW(edit, MF_STRING, edit_undo, L"&Undo\tCtrl+Z");
        AppendMenuW(edit, MF_STRING, edit_redo, L"&Redo\tCtrl+Y");
        AppendMenuW(edit, MF_STRING, edit_duplicate, L"&Duplicate\tCtrl+D");
        AppendMenuW(edit, MF_STRING, edit_delete, L"&Delete\tDel");
        AppendMenuW(menu_, MF_POPUP, reinterpret_cast<UINT_PTR>(edit), L"&Edit");
        HMENU view = CreatePopupMenu();
        AppendMenuW(view, MF_STRING, view_fit, L"Fit &arena\tHome");
        AppendMenuW(view, MF_STRING, view_selection, L"Frame &selection\tF");
        AppendMenuW(view, MF_STRING, view_match, L"&Match view");
        AppendMenuW(view, MF_STRING, view_bounds, L"Show all &bounds");
        AppendMenuW(view, MF_STRING | MF_CHECKED, view_blockout, L"&Blockout aids (reload tuning)");
        AppendMenuW(menu_, MF_POPUP, reinterpret_cast<UINT_PTR>(view), L"&View");
        AppendMenuW(menu_, MF_STRING, help_controls, L"&Help");
        SetMenu(window_, menu_);
        const ACCEL keys[] = {
            {FVIRTKEY | FCONTROL, 'N', file_new}, {FVIRTKEY | FCONTROL, 'O', file_open},
            {FVIRTKEY | FCONTROL, 'S', file_save},
            {FVIRTKEY | FCONTROL | FSHIFT, 'S', file_save_as},
            {FVIRTKEY | FCONTROL, 'M', file_save_user_stage},
            {FVIRTKEY | FCONTROL, 'Z', edit_undo}, {FVIRTKEY | FCONTROL, 'Y', edit_redo},
            {FVIRTKEY | FCONTROL, 'D', edit_duplicate}
        };
        accelerators_ = CreateAcceleratorTableW(const_cast<ACCEL*>(keys), static_cast<int>(std::size(keys)));
    }

    void Editor::create_controls()
    {
        dpi_ = editor_window_dpi(window_);
        font_ = editor_font(dpi_);
        create_menu();
        toolbar_.push_back(control(L"BUTTON", L"New", WS_TABSTOP, file_new));
        toolbar_.push_back(control(L"BUTTON", L"Open...", WS_TABSTOP, file_open));
        toolbar_.push_back(control(L"BUTTON", L"Save", WS_TABSTOP, file_save));
        toolbar_.push_back(control(L"BUTTON", L"Undo", WS_TABSTOP, edit_undo));
        toolbar_.push_back(control(L"BUTTON", L"Redo", WS_TABSTOP, edit_redo));
        palette_ = control(WC_COMBOBOXW, L"Object to add", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, palette);
        const wchar_t* names[] = {L"Paintable block", L"Solid block", L"Jump-through platform",
            L"Ramp rising right", L"Ramp rising left", L"Decoration", L"Team A spawn", L"Team B spawn",
            L"Moving paintable block", L"Moving paintable platform", L"Scored slab rising right", L"Scored slab rising left",
            L"Team gate"};
        for (const wchar_t* name : names) SendMessageW(palette_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        SendMessageW(palette_, CB_SETCURSEL, 0, 0);
        toolbar_.push_back(control(L"BUTTON", L"+ Add", WS_TABSTOP, add_object));
        toolbar_.push_back(control(L"BUTTON", L"Match view", WS_TABSTOP, view_match));
        toolbar_.push_back(control(L"BUTTON", L"Fit arena", WS_TABSTOP, view_fit));
        grid_ = control(WC_COMBOBOXW, L"Grid snap", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, grid_size);
        for (const wchar_t* name : {L"Snap 1", L"Snap 10", L"Snap 25", L"Snap 50", L"Snap 100"})
            SendMessageW(grid_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        SendMessageW(grid_, CB_SETCURSEL, 2, 0);
        object_heading_ = control(L"STATIC", L"LEVEL CONTENTS", SS_LEFT, 0);
        inspector_heading_ = control(L"STATIC", L"PROPERTIES", SS_LEFT, 0);
        object_list_ = control(WC_LISTBOXW, L"Level contents", LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP | WS_BORDER, objects_list);
        SendMessageW(object_list_, LB_SETHORIZONTALEXTENT, pixels(650), 0);
        property_list_ = control(WC_LISTVIEWW, L"Properties", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP | WS_BORDER, properties_list);
        ListView_SetExtendedListViewStyle(property_list_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t*>(L"Property"); column.cx = pixels(155);
        ListView_InsertColumn(property_list_, 0, &column);
        column.pszText = const_cast<wchar_t*>(L"Value"); column.cx = pixels(150);
        ListView_InsertColumn(property_list_, 1, &column);
        property_label_ = control(L"STATIC", L"Select a property to edit", SS_LEFT, 0);
        value_edit_ = control(L"EDIT", L"", ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, property_value);
        SendMessageW(value_edit_, EM_SETLIMITTEXT, 32767, 0);
        apply_ = control(L"BUTTON", L"Apply", WS_TABSTOP, apply_property);
        revert_ = control(L"BUTTON", L"Revert", WS_TABSTOP, revert_property);
        hint_ = control(L"STATIC", L"Drag: move   White corner: resize\r\nWheel: zoom   Middle drag: pan\r\nF: selection   Home: fit arena\r\nShift: bypass snap   Arrows: nudge\r\nCyan: exposed scoring faces\r\nMovers: white anchor; blue travel;\r\ngold corridor; filled initial pose\r\nGeometry preview; no textures", SS_LEFT, 0);
        status_ = control(L"STATIC", L"", SS_LEFT, 0);
        canvas_ = CreateWindowExW(0, L"ColourWarsEditorCanvas", L"Level canvas",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, window_, nullptr, instance_, this);
        if (!canvas_) throw std::runtime_error("Could not create the level canvas.");
    }

    void Editor::layout()
    {
        // SetMenu sends WM_SIZE while WM_CREATE is still constructing the controls.
        if (!canvas_ || toolbar_.size() != 8) return;
        RECT client{}; GetClientRect(window_, &client);
        if (client.right <= 0 || client.bottom <= 0) { return; }
        const EditorWindowLayout positions = editor_window_layout(client.right, client.bottom, dpi_);
        const auto move = [](HWND control, const RECT& rectangle, int drop = 0)
        {
            MoveWindow(control, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
                rectangle.bottom - rectangle.top + drop, TRUE);
        };
        for (size_t index = 0; index < toolbar_.size(); ++index) { move(toolbar_[index], positions.toolbar[index]); }
        move(palette_, positions.palette, positions.combo_drop_height);
        move(grid_, positions.grid, positions.combo_drop_height);
        move(object_heading_, positions.object_heading);
        move(inspector_heading_, positions.inspector_heading);
        move(object_list_, positions.object_list);
        move(canvas_, positions.canvas);
        move(property_list_, positions.property_list);
        move(property_label_, positions.property_label);
        move(value_edit_, positions.value_edit);
        move(apply_, positions.apply);
        move(revert_, positions.revert);
        if (positions.show_hint) { move(hint_, positions.hint); }
        ShowWindow(hint_, positions.show_hint ? SW_SHOWNA : SW_HIDE);
        move(status_, positions.status);
        const int property_width = positions.property_list.right - positions.property_list.left;
        ListView_SetColumnWidth(property_list_, 0, property_width / 2);
        ListView_SetColumnWidth(property_list_, 1, std::max(1, property_width - property_width / 2 - pixels(20)));
        SendMessageW(object_list_, LB_SETHORIZONTALEXTENT, pixels(650), 0);
    }

    void Editor::change_dpi(UINT dpi, const RECT& suggested)
    {
        HFONT replacement = dpi != dpi_ ? editor_font(dpi) : nullptr;
        // A pointer gesture belongs to the old client geometry. Cancel its
        // preview without committing it or applying pending inspector text.
        cancel_drag();
        if (replacement)
        {
            HFONT previous = font_;
            font_ = replacement;
            dpi_ = dpi;
            EnumChildWindows(window_, [](HWND child, LPARAM font) -> BOOL
            {
                SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(font), TRUE);
                return TRUE;
            }, reinterpret_cast<LPARAM>(replacement));
            if (previous) { DeleteObject(previous); }
        }
        SetWindowPos(window_, nullptr, suggested.left, suggested.top,
            suggested.right - suggested.left, suggested.bottom - suggested.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        layout();
        RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }

    void Editor::error(const std::exception& exception)
    {
        MessageBoxW(window_, wide(exception.what()).c_str(), L"ColourWars Level Editor", MB_OK | MB_ICONERROR);
    }

    void Editor::set_status(const std::wstring& text)
    {
        SetWindowTextW(status_, text.c_str());
    }

    void Editor::refresh(bool keep_property)
    {
        refreshing_ = true;
        entries_ = document_.entries();
        draw_items_.clear();
        blockout_tuning_.reset();
        scoring_faces_.clear();
        try
        {
            blockout_tuning_ = load_blockout_tuning(std::filesystem::path(COLOURWARS_CONTENT_DIR) / "tuning.json",
                std::filesystem::path(COLOURWARS_EDITOR_DIR) / "movement-baseline.json");
            scoring_faces_ = scoring_faces(document_, *blockout_tuning_);
            blockout_error_.clear();
        }
        catch (const std::exception& exception) { blockout_error_ = wide(exception.what()); }
        content_catalog_ = load_content_catalog(std::filesystem::path(COLOURWARS_CONTENT_DIR));
        unresolved_ = document_.unresolved_resources(content_catalog_);
        SendMessageW(object_list_, WM_SETREDRAW, FALSE, 0);
        SendMessageW(object_list_, LB_RESETCONTENT, 0, 0);
        int selected_index = -1;
        for (size_t index = 0; index < entries_.size(); ++index)
        {
            const Entry& entry = entries_[index];
            const std::wstring label = wide(entry.label);
            SendMessageW(object_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            if (selection_ && same_entry(entry, *selection_))
            {
                selection_ = entry; selected_index = static_cast<int>(index);
            }
            if (entry.kind != EntryKind::level)
            {
                DrawItem item{entry, document_.rectangle(entry), {}};
                for (const Property& property : document_.properties(entry))
                    item.values[property.path.starts_with('/') ? property.path.substr(1) : property.path] = property.value;
                try { item.motion = document_.motion_guide(entry); }
                catch (const std::exception& exception) { blockout_error_ = wide(exception.what()); }
                item.slab = document_.slab_guide(entry);
                if (blockout_tuning_ && (entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b))
                {
                    item.rectangle.width = blockout_tuning_->player_width;
                    item.rectangle.height = blockout_tuning_->player_height;
                }
                draw_items_.push_back(std::move(item));
            }
        }
        if (selected_index < 0 && !entries_.empty()) { selection_ = entries_[0]; selected_index = 0; }
        SendMessageW(object_list_, LB_SETCURSEL, static_cast<WPARAM>(selected_index), 0);
        SendMessageW(object_list_, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(object_list_, nullptr, TRUE);
        refreshing_ = false;
        refresh_properties(keep_property);
        const std::wstring name = document_.path().empty() ? L"Untitled level" : document_.path().filename().wstring();
        const bool changed = document_.dirty() && (document_.can_undo() || !document_.path().empty());
        SetWindowTextW(window_, (name + (changed ? L" *" : L"") + L" - ColourWars Level Editor").c_str());
        EnableMenuItem(menu_, edit_undo, MF_BYCOMMAND | (document_.can_undo() ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu_, edit_redo, MF_BYCOMMAND | (document_.can_redo() ? MF_ENABLED : MF_GRAYED));
        EnableWindow(toolbar_[3], document_.can_undo()); EnableWindow(toolbar_[4], document_.can_redo());
        const std::vector<std::string> errors = document_.validate();
        const std::wstring ready = L"Ready  |  " + std::to_wstring(draw_items_.size()) + L" items  |  ";
        if (!errors.empty()) set_status(L"Needs attention: " + wide(errors.front()));
        else if (!unresolved_.empty())
            set_status(ready + std::to_wstring(unresolved_.size()) + (unresolved_.size() == 1 ? L" name" : L" names")
                + L" the game will refuse: " + wide(unresolved_.front()));
        else if (!content_catalog_.problems.empty())
            set_status(ready + L"Resource names unchecked: " + wide(content_catalog_.problems.front()));
        else set_status(ready + destination_guidance(document_.path(), content_catalog_, false));
        InvalidateRect(canvas_, nullptr, FALSE);
    }

    void Editor::refresh_properties(bool keep_property)
    {
        std::string previous;
        if (keep_property && property_index_ >= 0 && static_cast<size_t>(property_index_) < properties_.size())
            previous = properties_[static_cast<size_t>(property_index_)].path;
        refreshing_ = true;
        properties_ = selection_ ? document_.properties(*selection_) : std::vector<Property>{};
        ListView_DeleteAllItems(property_list_);
        property_index_ = -1;
        for (size_t index = 0; index < properties_.size(); ++index)
        {
            const Property& property = properties_[index];
            std::wstring name = wide(property.label), value = wide(property.value);
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(index); item.pszText = name.data();
            ListView_InsertItem(property_list_, &item);
            ListView_SetItemText(property_list_, static_cast<int>(index), 1, value.data());
            if (property.path == previous) property_index_ = static_cast<int>(index);
        }
        if (property_index_ < 0 && !properties_.empty()) property_index_ = 0;
        if (property_index_ >= 0)
            ListView_SetItemState(property_list_, property_index_, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        refreshing_ = false;
        select_property(property_index_);
    }

    void Editor::select_property(int index)
    {
        property_index_ = index;
        const bool exists = index >= 0 && static_cast<size_t>(index) < properties_.size();
        EnableWindow(value_edit_, exists); EnableWindow(apply_, exists); EnableWindow(revert_, exists);
        if (!exists) { SetWindowTextW(value_edit_, L""); SetWindowTextW(property_label_, L"Select a property to edit"); return; }
        const Property& property = properties_[static_cast<size_t>(index)];
        SetWindowTextW(value_edit_, wide(property.value).c_str());
        SetWindowTextW(property_label_, wide(property.label + (property.boolean ? " (true / false)" : "")).c_str());
    }

    bool Editor::pending() const
    {
        return property_index_ >= 0 && static_cast<size_t>(property_index_) < properties_.size()
            && window_text(value_edit_) != wide(properties_[static_cast<size_t>(property_index_)].value);
    }

    bool Editor::apply_pending()
    {
        if (!pending() || !selection_) return true;
        try
        {
            document_.set_property(*selection_, properties_[static_cast<size_t>(property_index_)].path, narrow(window_text(value_edit_)));
            refresh(true);
            return true;
        }
        catch (const std::exception& exception) { error(exception); SetFocus(value_edit_); return false; }
    }

    void Editor::select(const Entry& entry)
    {
        if (!apply_pending())
        {
            for (size_t index = 0; index < entries_.size(); ++index)
                if (selection_ && same_entry(entries_[index], *selection_))
                    SendMessageW(object_list_, LB_SETCURSEL, index, 0);
            return;
        }
        selection_ = entry;
        refresh();
    }

    std::optional<std::filesystem::path> Editor::choose_file(bool save_as)
    {
        std::array<wchar_t, 32768> buffer{};
        if (save_as)
        {
            const std::wstring filename = document_.path().empty() ? L"new_level.json" : document_.path().filename().wstring();
            wcscpy_s(buffer.data(), buffer.size(), filename.c_str());
        }
        std::filesystem::path directory = document_.path().empty()
            ? std::filesystem::path(COLOURWARS_CONTENT_DIR) / L"levels" : document_.path().parent_path();
        directory = std::filesystem::absolute(directory).make_preferred();
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = window_;
        dialog.lpstrFilter = L"ColourWars level (*.json)\0*.json\0All files\0*.*\0";
        dialog.lpstrFile = buffer.data(); dialog.nMaxFile = static_cast<DWORD>(buffer.size());
        dialog.lpstrInitialDir = directory.c_str(); dialog.lpstrDefExt = L"json";
        dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST
            | (save_as ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
        if (!editor_file_dialog(dialog, save_as)) { return std::nullopt; }
        return std::filesystem::path(buffer.data());
    }

    bool Editor::confirm_unresolved()
    {
        if (unresolved_.empty()) return true;
        // Warn, never block: a stage is often saved midway through adding its art.
        std::wstring text = L"The game will refuse this stage at startup:\n";
        constexpr size_t shown = 8;
        for (size_t index = 0; index < std::min(shown, unresolved_.size()); ++index)
            text += L"\n- " + wide(unresolved_[index]);
        if (unresolved_.size() > shown) text += L"\n- and " + std::to_wstring(unresolved_.size() - shown) + L" more";
        text += L"\n\nNames are checked against game/content/manifest.json and the sheets and banks it lists. Save anyway?";
        return MessageBoxW(window_, text.c_str(), L"Unresolved resource names",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
    }

    bool Editor::save(bool save_as)
    {
        if (!apply_pending()) return false;
        if (!confirm_unresolved()) return false;
        std::filesystem::path path = document_.path();
        if (save_as || path.empty())
        {
            const std::optional<std::filesystem::path> selected = choose_file(true);
            if (!selected) return false;
            path = *selected;
        }
        document_.save(path);
        refresh(true);
        set_status(L"Saved " + path.filename().wstring() + L"  |  " +
            destination_guidance(path, content_catalog_, true) + L"  |  " + path.wstring());
        return true;
    }

    std::optional<std::filesystem::path> Editor::choose_user_stage(const std::filesystem::path& folder)
    {
        std::array<wchar_t, 32768> buffer{};
        const std::wstring filename = wide(suggested_user_stage_name(document_, content_catalog_.stages)) + L".json";
        wcscpy_s(buffer.data(), buffer.size(), filename.c_str());
        const std::filesystem::path directory = std::filesystem::absolute(folder).lexically_normal().make_preferred();
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = window_;
        dialog.lpstrFilter = L"ColourWars level (*.json)\0*.json\0";
        dialog.lpstrFile = buffer.data(); dialog.nMaxFile = static_cast<DWORD>(buffer.size());
        dialog.lpstrInitialDir = directory.c_str(); dialog.lpstrDefExt = L"json";
        dialog.lpstrTitle = L"Save to My Stages";
        // The overwrite question is asked below, in the game's own terms.
        dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
        if (!editor_file_dialog(dialog, true)) { return std::nullopt; }
        return std::filesystem::path(buffer.data());
    }

    bool Editor::save_user_stage()
    {
        if (!apply_pending()) return false;
        if (!content_catalog_.manifest_read)
            throw std::runtime_error("The shipped stage names could not be read from game/content/manifest.json, "
                "so the editor cannot check that this name is free. Use Save As instead.");
        if (!confirm_unresolved()) return false;
        const std::filesystem::path folder = user_stage_folder();
        // The folder appears only on this explicit save. The dialog needs it to
        // open there, so a cancelled or refused save takes an empty one away again.
        std::error_code error;
        // The game folder too may be new (the game has never run here); take
        // away only what this command made, and only while it is empty.
        const bool parent_existed = std::filesystem::exists(folder.parent_path(), error);
        const bool created = std::filesystem::create_directories(folder, error);
        if (error) throw std::runtime_error("Cannot create " + narrow(folder.wstring()) + ": " + error.message());
        const auto tidy = [&folder, parent_existed]
        {
            std::error_code ignored;
            if (std::filesystem::remove(folder, ignored) && !parent_existed)
                std::filesystem::remove(folder.parent_path(), ignored);
        };
        bool saved = false;
        try
        {
            const std::optional<std::filesystem::path> chosen = choose_user_stage(folder);
            if (chosen)
            {
                const UserStageTarget target = check_user_stage_target(folder, *chosen, content_catalog_.stages);
                const std::wstring question = L"Replace your stage '" + wide(target.name) + L"' in My Stages?\n\n"
                    L"The game plays the new version the next time Stage Select opens from the main menu.";
                if (!target.replaces || MessageBoxW(window_, question.c_str(), L"Save to My Stages",
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
                {
                    colourwars_editor::save_user_stage(document_, folder, target);
                    saved = true;
                    refresh(true);
                    set_status(L"Saved to My Stages as '" + wide(target.name) + L"'  |  " +
                        destination_guidance(target.file, content_catalog_, true) + L"  |  " + target.file.wstring());
                }
            }
        }
        catch (...)
        {
            if (created && !saved) tidy();
            throw;
        }
        if (created && !saved) tidy();
        return saved;
    }

    bool Editor::confirm_discard()
    {
        const bool changed = document_.dirty() && (document_.can_undo() || !document_.path().empty());
        if (!pending() && !changed) return true;
        const int answer = MessageBoxW(window_, L"Save your changes before continuing?",
            L"Unsaved level", MB_YESNOCANCEL | MB_ICONQUESTION);
        if (answer == IDCANCEL) return false;
        return answer == IDNO || save(false);
    }

    void Editor::command(int id)
    {
        if (dragging_ || panning_) cancel_drag();
        if (id == file_exit) { SendMessageW(window_, WM_CLOSE, 0, 0); return; }
        if (id == file_new || id == file_open)
        {
            if (!confirm_discard()) return;
            if (id == file_new) document_.create();
            else
            {
                const std::optional<std::filesystem::path> path = choose_file(false);
                if (!path) return;
                document_.load(*path);
            }
            selection_.reset(); refresh(); command(view_match); return;
        }
        if (id == file_save || id == file_save_as) { save(id == file_save_as); return; }
        if (id == file_save_user_stage) { save_user_stage(); return; }
        if (id == revert_property) { select_property(property_index_); return; }
        if (id == help_controls)
        {
            MessageBoxW(window_,
                L"Open a level from game/content/levels, or start a new one.\n\n"
                L"Select objects on the canvas or in Level Contents. Drag to move; use the white bottom-right corner to resize. "
                L"Select a property, edit its value and click Apply (or press Enter). Changes apply before selection/save; Escape or Revert discards an uncommitted value.\n\n"
                L"Wheel zooms around the pointer. Middle drag pans. F frames selection; Home fits the arena. "
                L"Arrows nudge by the snap amount; Shift bypasses snap. Ctrl+D duplicates, Delete removes, Ctrl+Z/Y undo/redo. "
                L"New objects appear in the centre of the current view.\n\n"
                L"Moving paintables use the rectangle as their left anchor. Drag/resize edits that anchor; travel stays in world units. "
                L"Blue shows full travel, gold the source-tuned standing corridor, and the filled body its initial phase. "
                L"Duplicate places a copy below the complete corridor; blocked edits/copies are refused. Guides do not simulate riding or qualify runtime rate limits.\n\n"
                L"Scored slabs have a fixed 160 by 80 slope and 24-unit normal thickness. Drag moves them; their slab/rise property selects left or right. They cannot be resized.\n\n"
                L"Cyan edges mark exposed scoring faces (authored edges when aids are off). Spawn boxes follow source tuning. "
                L"Bounds are selectable in the list. View > Show all bounds shows every rectangle. This is a geometry preview, without textures or gameplay.\n\n"
                L"Save to SOURCE content and rebuild the game. A NEW filename also needs its stem in game/content/manifest.json "
                L"and its relative path in cmake/runtime_assets.cmake. The opt-in ColourWarsRegisterStage helper previews both entries; --apply registers them. "
                L"Existing registered levels need no registration.\n\n"
                L"To play a level without rebuilding, use File > Save to My Stages (Ctrl+M). It saves into %LOCALAPPDATA%\\ColourWars\\stages, "
                L"which the game reads at startup and whenever Stage Select opens from its main menu. The name must not be a shipped stage's. "
                L"Asset names must match your content: the status line names any the game would refuse, and Save asks first. See docs/level-editor.md.",
                L"Using the level editor", MB_OK | MB_ICONINFORMATION);
            return;
        }
        if (!apply_pending()) return;
        switch (id)
        {
        case edit_undo: document_.undo(); refresh(true); break;
        case edit_redo: document_.redo(); refresh(true); break;
        case apply_property: break;
        case edit_duplicate:
            if (selection_) { selection_ = document_.duplicate(*selection_); refresh(); } break;
        case edit_delete:
            if (selection_) { document_.erase(*selection_); selection_.reset(); refresh(); } break;
        case view_fit: fit(document_.rectangle(Entry{EntryKind::bounds, 0, ""})); break;
        case view_match: fit(document_.rectangle(Entry{EntryKind::bounds, 3, ""})); break;
        case view_selection:
            if (selection_)
            {
                for (const DrawItem& item : draw_items_)
                    if (same_entry(item.entry, *selection_))
                    {
                        Rect area = item.rectangle;
                        if (item.motion)
                        {
                            area = item.motion->standing_envelope;
                            area.height = item.motion->travel_envelope.y + item.motion->travel_envelope.height - area.y;
                        }
                        fit(area); break;
                    }
            }
            break;
        case view_blockout:
            show_blockout_ = !show_blockout_;
            CheckMenuItem(menu_, view_blockout, MF_BYCOMMAND | (show_blockout_ ? MF_CHECKED : MF_UNCHECKED));
            refresh(true); break;
        case view_bounds:
            show_bounds_ = !show_bounds_; CheckMenuItem(menu_, view_bounds, MF_BYCOMMAND | (show_bounds_ ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(canvas_, nullptr, FALSE); break;
        case add_object:
        {
            const char* types[] = {"StructurePaintable", "Structure", "StructureJumpThrough", "StructureRampRight",
                "StructureRampLeft", "Visual", "SpawnA", "SpawnB", "MovingPaintable", "MovingJumpThrough", "FixedSlabRight", "FixedSlabLeft",
                "TeamGate"};
            const LRESULT index = SendMessageW(palette_, CB_GETCURSEL, 0, 0);
            if (index < 0 || index >= static_cast<LRESULT>(std::size(types))) break;
            RECT client{}; GetClientRect(canvas_, &client);
            const Rect rectangle{snapped(view_x_ + client.right / (2.0 * zoom_) - 100.0),
                snapped(view_y_ + client.bottom / (2.0 * zoom_) - 50.0), 200.0, 100.0};
            selection_ = document_.add_object(types[index], rectangle); refresh(); SetFocus(canvas_); break;
        }
        default: break;
        }
    }

    LRESULT Editor::message(UINT message_id, WPARAM wparam, LPARAM lparam)
    {
        switch (message_id)
        {
        case WM_CREATE: create_controls(); return 0;
        case WM_SIZE: layout(); return 0;
        case WM_GETMINMAXINFO:
        {
            const RECT work = editor_monitor_work_area(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST));
            const SIZE minimum = minimum_editor_size(dpi_, work);
            reinterpret_cast<MINMAXINFO*>(lparam)->ptMinTrackSize = POINT{minimum.cx, minimum.cy};
            return 0;
        }
        case WM_DPICHANGED:
            if (LOWORD(wparam) != 0 && lparam != 0)
            { change_dpi(LOWORD(wparam), *reinterpret_cast<const RECT*>(lparam)); }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wparam) == objects_list && HIWORD(wparam) == LBN_SELCHANGE && !refreshing_)
            {
                const LRESULT index = SendMessageW(object_list_, LB_GETCURSEL, 0, 0);
                if (index >= 0 && static_cast<size_t>(index) < entries_.size())
                {
                    const Entry entry = entries_[static_cast<size_t>(index)]; select(entry);
                }
                return 0;
            }
            if (LOWORD(wparam) == grid_size && HIWORD(wparam) == CBN_SELCHANGE)
            {
                const int sizes[] = {1, 10, 25, 50, 100};
                const LRESULT index = SendMessageW(grid_, CB_GETCURSEL, 0, 0);
                if (index >= 0 && index < 5) snap_ = sizes[index];
                InvalidateRect(canvas_, nullptr, FALSE); return 0;
            }
            if (HIWORD(wparam) == 0 || HIWORD(wparam) == 1) command(LOWORD(wparam));
            return 0;
        case WM_NOTIFY:
        {
            const NMHDR* header = reinterpret_cast<const NMHDR*>(lparam);
            if (header->hwndFrom == property_list_ && header->code == LVN_ITEMCHANGING && !refreshing_)
            {
                const NMLISTVIEW* change = reinterpret_cast<const NMLISTVIEW*>(lparam);
                if (((change->uOldState ^ change->uNewState) & LVIS_SELECTED) && pending())
                {
                    // Preserve unfinished text; Apply/Revert explicitly before changing rows.
                    set_status(L"Apply or Revert the property value before choosing another property.");
                    return TRUE;
                }
            }
            if (header->hwndFrom == property_list_ && header->code == LVN_ITEMCHANGED && !refreshing_)
            {
                const NMLISTVIEW* change = reinterpret_cast<const NMLISTVIEW*>(lparam);
                if ((change->uNewState & LVIS_SELECTED) && !(change->uOldState & LVIS_SELECTED)) select_property(change->iItem);
            }
            return 0;
        }
        case WM_CLOSE:
            if (confirm_discard()) DestroyWindow(window_); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: break;
        }
        return DefWindowProcW(window_, message_id, wparam, lparam);
    }

    LRESULT CALLBACK Editor::window_proc(HWND window, UINT message_id, WPARAM wparam, LPARAM lparam)
    {
        Editor* editor = reinterpret_cast<Editor*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message_id == WM_NCCREATE)
        {
            editor = static_cast<Editor*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
            editor->window_ = window; SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(editor));
        }
        if (!editor) return DefWindowProcW(window, message_id, wparam, lparam);
        try { return editor->message(message_id, wparam, lparam); }
        catch (const std::exception& exception) { editor->error(exception); return message_id == WM_CREATE ? -1 : 0; }
    }

    void Editor::create_window(int show, const std::filesystem::path& initial_path)
    {
        enable_editor_dpi_awareness();
        INITCOMMONCONTROLSEX common{sizeof(common), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&common);
        WNDCLASSEXW type{};
        type.cbSize = sizeof(type); type.hInstance = instance_; type.lpfnWndProc = window_proc;
        type.hCursor = LoadCursorW(nullptr, IDC_ARROW); type.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        type.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1); type.lpszClassName = L"ColourWarsLevelEditor";
        if (!RegisterClassExW(&type) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        { throw std::runtime_error("Could not register the editor window."); }
        type.lpfnWndProc = canvas_proc; type.lpszClassName = L"ColourWarsEditorCanvas"; type.hbrBackground = nullptr;
        if (!RegisterClassExW(&type) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        { throw std::runtime_error("Could not register the canvas."); }
        const HMONITOR monitor = MonitorFromPoint(POINT{}, MONITOR_DEFAULTTOPRIMARY);
        dpi_ = editor_monitor_dpi(monitor);
        const RECT initial = initial_editor_rectangle(dpi_, editor_monitor_work_area(monitor));
        if (!CreateWindowExW(WS_EX_CONTROLPARENT, L"ColourWarsLevelEditor", L"ColourWars Level Editor",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, initial.left, initial.top,
            initial.right - initial.left, initial.bottom - initial.top,
            nullptr, nullptr, instance_, this)) throw std::runtime_error("Could not open the editor.");
        if (initial_path.empty()) document_.create(); else document_.load(initial_path);
        refresh(); layout(); command(view_match);
        ShowWindow(window_, show); UpdateWindow(window_);
    }

    int Editor::run(int show, const std::filesystem::path& initial_path)
    {
        create_window(show, initial_path);
        MSG message_data{};
        while (GetMessageW(&message_data, nullptr, 0, 0) > 0)
        {
            const HWND focus = GetFocus();
            if (message_data.message == WM_KEYDOWN && focus == value_edit_)
            {
                if (message_data.wParam == VK_RETURN) { apply_pending(); continue; }
                if (message_data.wParam == VK_ESCAPE) { select_property(property_index_); continue; }
                // Text fields keep native text undo/select/copy/delete behavior.
                if ((GetKeyState(VK_CONTROL) & 0x8000) && (message_data.wParam == 'Z' || message_data.wParam == 'Y'))
                { TranslateMessage(&message_data); DispatchMessageW(&message_data); continue; }
            }
            if (message_data.message == WM_KEYDOWN && focus == object_list_)
            {
                if (message_data.wParam == VK_DELETE) { SendMessageW(window_, WM_COMMAND, edit_delete, 0); continue; }
                if (message_data.wParam == 'F') { SendMessageW(window_, WM_COMMAND, view_selection, 0); continue; }
            }
            if (!TranslateAcceleratorW(window_, accelerators_, &message_data)
                && !(message_data.message == WM_KEYDOWN && message_data.wParam == VK_TAB && IsDialogMessageW(window_, &message_data)))
            { TranslateMessage(&message_data); DispatchMessageW(&message_data); }
        }
        return static_cast<int>(message_data.wParam);
    }
}
