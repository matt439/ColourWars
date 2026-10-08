#include "editor/level_document.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace
{
    using namespace colourwars_editor;

    enum Command : int
    {
        file_new = 100, file_open, file_save, file_save_as, file_exit,
        edit_undo, edit_redo, edit_duplicate, edit_delete,
        view_fit, view_selection, view_match, view_bounds, help_controls,
        add_object, apply_property, revert_property, objects_list,
        properties_list, property_value, palette, grid_size
    };

    std::wstring wide(const std::string& value)
    {
        if (value.empty()) return {};
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (!count) throw std::runtime_error("Text is not valid UTF-8.");
        std::wstring result(static_cast<size_t>(count), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), count);
        return result;
    }

    std::string narrow(const std::wstring& value)
    {
        if (value.empty()) return {};
        const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (!count) throw std::runtime_error("Text contains an invalid Unicode character.");
        std::string result(static_cast<size_t>(count), '\0');
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), count, nullptr, nullptr);
        return result;
    }

    std::wstring window_text(HWND window)
    {
        std::wstring result(static_cast<size_t>(GetWindowTextLengthW(window)) + 1, L'\0');
        GetWindowTextW(window, result.data(), static_cast<int>(result.size()));
        result.resize(wcslen(result.c_str()));
        return result;
    }

    bool same_entry(const Entry& a, const Entry& b)
    {
        return a.kind == b.kind && a.index == b.index;
    }

    bool is_spawn(const Entry& entry)
    {
        return entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b;
    }

    void fill(HDC dc, const RECT& rectangle, COLORREF colour)
    {
        HBRUSH brush = CreateSolidBrush(colour);
        FillRect(dc, &rectangle, brush);
        DeleteObject(brush);
    }

    void line(HDC dc, int x1, int y1, int x2, int y2, COLORREF colour,
        int width = 1, int style = PS_SOLID)
    {
        HPEN pen = CreatePen(style, width, colour);
        HGDIOBJ previous = SelectObject(dc, pen);
        MoveToEx(dc, x1, y1, nullptr);
        LineTo(dc, x2, y2);
        SelectObject(dc, previous);
        DeleteObject(pen);
    }

    void outline(HDC dc, const RECT& rectangle, COLORREF colour, int width = 1,
        int style = PS_SOLID)
    {
        line(dc, rectangle.left, rectangle.top, rectangle.right, rectangle.top, colour, width, style);
        line(dc, rectangle.right, rectangle.top, rectangle.right, rectangle.bottom, colour, width, style);
        line(dc, rectangle.right, rectangle.bottom, rectangle.left, rectangle.bottom, colour, width, style);
        line(dc, rectangle.left, rectangle.bottom, rectangle.left, rectangle.top, colour, width, style);
    }

    struct DrawItem
    {
        Entry entry;
        Rect rectangle;
        std::map<std::string, std::string> values;
    };

    class Editor
    {
    public:
        explicit Editor(HINSTANCE instance) : instance_(instance) {}
        int run(int show, const std::filesystem::path& initial_path);

    private:
        HINSTANCE instance_;
        HWND window_ = nullptr;
        HWND canvas_ = nullptr;
        HWND object_list_ = nullptr;
        HWND property_list_ = nullptr;
        HWND value_edit_ = nullptr;
        HWND property_label_ = nullptr;
        HWND status_ = nullptr;
        HWND palette_ = nullptr;
        HWND grid_ = nullptr;
        HWND object_heading_ = nullptr;
        HWND inspector_heading_ = nullptr;
        HWND hint_ = nullptr;
        HWND apply_ = nullptr;
        HWND revert_ = nullptr;
        HFONT font_ = nullptr;
        HMENU menu_ = nullptr;
        HACCEL accelerators_ = nullptr;
        std::vector<HWND> toolbar_;
        LevelDocument document_;
        std::vector<Entry> entries_;
        std::vector<DrawItem> draw_items_;
        std::vector<Property> properties_;
        std::optional<Entry> selection_;
        int property_index_ = -1;
        bool refreshing_ = false;
        bool show_bounds_ = false;
        bool dragging_ = false;
        bool resizing_ = false;
        bool panning_ = false;
        POINT mouse_start_{};
        Rect drag_original_{};
        Rect drag_preview_{};
        double view_x_ = 0.0;
        double view_y_ = 0.0;
        double zoom_ = 0.2;
        int snap_ = 25;

        static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK canvas_proc(HWND, UINT, WPARAM, LPARAM);
        LRESULT message(UINT, WPARAM, LPARAM);
        LRESULT canvas_message(UINT, WPARAM, LPARAM);
        HWND control(const wchar_t*, const wchar_t*, DWORD, int);
        void create_controls();
        void create_menu();
        void layout();
        void command(int);
        void refresh(bool keep_property = false);
        void refresh_properties(bool keep_property);
        void select_property(int);
        bool apply_pending();
        bool pending() const;
        bool confirm_discard();
        bool save(bool save_as);
        std::optional<std::filesystem::path> choose_file(bool save_as);
        void select(const Entry&);
        void set_status(const std::wstring&);
        void error(const std::exception&);
        void paint(HDC, const RECT&);
        void draw_item(HDC, const DrawItem&);
        void fit(const Rect&);
        RECT screen_rect(const Rect&) const;
        POINT screen_point(double, double) const;
        std::optional<Entry> hit(POINT) const;
        double snapped(double) const;
        void cancel_drag();
        void nudge(int, int);
    };

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
        AppendMenuW(menu_, MF_POPUP, reinterpret_cast<UINT_PTR>(view), L"&View");
        AppendMenuW(menu_, MF_STRING, help_controls, L"&Help");
        SetMenu(window_, menu_);
        const ACCEL keys[] = {
            {FVIRTKEY | FCONTROL, 'N', file_new}, {FVIRTKEY | FCONTROL, 'O', file_open},
            {FVIRTKEY | FCONTROL, 'S', file_save},
            {FVIRTKEY | FCONTROL | FSHIFT, 'S', file_save_as},
            {FVIRTKEY | FCONTROL, 'Z', edit_undo}, {FVIRTKEY | FCONTROL, 'Y', edit_redo},
            {FVIRTKEY | FCONTROL, 'D', edit_duplicate}
        };
        accelerators_ = CreateAcceleratorTableW(const_cast<ACCEL*>(keys), static_cast<int>(std::size(keys)));
    }

    void Editor::create_controls()
    {
        font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        create_menu();
        toolbar_.push_back(control(L"BUTTON", L"New", WS_TABSTOP, file_new));
        toolbar_.push_back(control(L"BUTTON", L"Open...", WS_TABSTOP, file_open));
        toolbar_.push_back(control(L"BUTTON", L"Save", WS_TABSTOP, file_save));
        toolbar_.push_back(control(L"BUTTON", L"Undo", WS_TABSTOP, edit_undo));
        toolbar_.push_back(control(L"BUTTON", L"Redo", WS_TABSTOP, edit_redo));
        palette_ = control(WC_COMBOBOXW, L"Object to add", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, palette);
        const wchar_t* names[] = {L"Paintable block", L"Solid block", L"Jump-through platform",
            L"Ramp rising right", L"Ramp rising left", L"Decoration", L"Team A spawn", L"Team B spawn"};
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
        SendMessageW(object_list_, LB_SETHORIZONTALEXTENT, 650, 0);
        property_list_ = control(WC_LISTVIEWW, L"Properties", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP | WS_BORDER, properties_list);
        ListView_SetExtendedListViewStyle(property_list_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t*>(L"Property"); column.cx = 155;
        ListView_InsertColumn(property_list_, 0, &column);
        column.pszText = const_cast<wchar_t*>(L"Value"); column.cx = 150;
        ListView_InsertColumn(property_list_, 1, &column);
        property_label_ = control(L"STATIC", L"Select a property to edit", SS_LEFT, 0);
        value_edit_ = control(L"EDIT", L"", ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, property_value);
        SendMessageW(value_edit_, EM_SETLIMITTEXT, 32767, 0);
        apply_ = control(L"BUTTON", L"Apply", WS_TABSTOP, apply_property);
        revert_ = control(L"BUTTON", L"Revert", WS_TABSTOP, revert_property);
        hint_ = control(L"STATIC", L"Drag to move. Drag the white corner to resize.\r\n\r\nWheel: zoom   Middle drag: pan\r\nF: frame selection   Home: fit arena\r\nShift: bypass snap   Arrows: nudge\r\n\r\nCyan edges are paintable.\r\nPreview shows geometry, not textures.", SS_LEFT, 0);
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
        const int width = client.right, height = client.bottom;
        const int left = 255, right = 335, top = 86, bottom = 28;
        int x = 10;
        for (size_t index = 0; index < 5; ++index)
        {
            MoveWindow(toolbar_[index], x, 10, 66, 30, TRUE); x += 72;
        }
        MoveWindow(palette_, x + 8, 10, 205, 300, TRUE); x += 219;
        MoveWindow(toolbar_[5], x, 10, 70, 30, TRUE); x += 84;
        MoveWindow(toolbar_[6], x, 10, 100, 30, TRUE); x += 106;
        MoveWindow(toolbar_[7], x, 10, 85, 30, TRUE); x += 98;
        MoveWindow(grid_, x, 10, 100, 220, TRUE);
        MoveWindow(object_heading_, 12, 59, left - 22, 22, TRUE);
        MoveWindow(inspector_heading_, width - right + 12, 59, right - 22, 22, TRUE);
        MoveWindow(object_list_, 10, top, left - 20, height - top - bottom - 10, TRUE);
        MoveWindow(canvas_, left, 51, width - left - right, height - 51 - bottom - 10, TRUE);
        const int panel_x = width - right + 12;
        const int inspector_height = std::max(180, height - top - bottom - 310);
        MoveWindow(property_list_, panel_x, top, right - 24, inspector_height, TRUE);
        const int edit_y = top + inspector_height + 12;
        MoveWindow(property_label_, panel_x, edit_y, right - 24, 24, TRUE);
        MoveWindow(value_edit_, panel_x, edit_y + 28, right - 24, 28, TRUE);
        MoveWindow(apply_, panel_x, edit_y + 65, 94, 30, TRUE);
        MoveWindow(revert_, panel_x + 102, edit_y + 65, 94, 30, TRUE);
        MoveWindow(hint_, panel_x, edit_y + 113, right - 24, 190, TRUE);
        MoveWindow(status_, 12, height - bottom + 2, width - 24, 24, TRUE);
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
        set_status(errors.empty() ? L"Ready  |  " + std::to_wstring(draw_items_.size()) + L" items  |  Save source levels, then rebuild the game to play changes."
            : L"Needs attention: " + wide(errors.front()));
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
        const BOOL accepted = save_as ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
        if (!accepted)
        {
            if (CommDlgExtendedError()) throw std::runtime_error("The file dialog could not be opened.");
            return std::nullopt;
        }
        return std::filesystem::path(buffer.data());
    }

    bool Editor::save(bool save_as)
    {
        if (!apply_pending()) return false;
        std::filesystem::path path = document_.path();
        if (save_as || path.empty())
        {
            const std::optional<std::filesystem::path> selected = choose_file(true);
            if (!selected) return false;
            path = *selected;
        }
        document_.save(path);
        refresh(true);
        set_status(L"Saved " + path.wstring() + L"  |  Rebuild to stage changes. New filenames also need a manifest entry (see Help).");
        return true;
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

    void Editor::fit(const Rect& rectangle)
    {
        RECT client{}; GetClientRect(canvas_, &client);
        zoom_ = std::clamp(std::min((client.right - 80.0) / std::max(rectangle.width, 100.0),
            (client.bottom - 80.0) / std::max(rectangle.height, 100.0)), 0.005, 10.0);
        view_x_ = rectangle.x + rectangle.width / 2.0 - client.right / (2.0 * zoom_);
        view_y_ = rectangle.y + rectangle.height / 2.0 - client.bottom / (2.0 * zoom_);
        InvalidateRect(canvas_, nullptr, FALSE);
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
                L"Cyan edges mark paintable faces. Spawn markers show player top-left positions. "
                L"Bounds are selectable in the list. View > Show all bounds shows every rectangle. This is a geometry preview, without textures or gameplay.\n\n"
                L"Save to SOURCE content and rebuild the game. A NEW filename also needs its stem in game/content/manifest.json "
                L"and its relative path in cmake/release_assets.cmake. Existing registered levels need no registration. "
                L"Asset names must match your content. See docs/level-editor.md.",
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
        case view_selection: if (selection_) fit(document_.rectangle(*selection_)); break;
        case view_bounds:
            show_bounds_ = !show_bounds_; CheckMenuItem(menu_, view_bounds, MF_BYCOMMAND | (show_bounds_ ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(canvas_, nullptr, FALSE); break;
        case add_object:
        {
            const char* types[] = {"StructurePaintable", "Structure", "StructureJumpThrough", "StructureRampRight",
                "StructureRampLeft", "Visual", "SpawnA", "SpawnB"};
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

    POINT Editor::screen_point(double x, double y) const
    {
        // Clip far-off geometry before converting to GDI's integer coordinates.
        return {static_cast<LONG>(std::clamp((x - view_x_) * zoom_, -1000000.0, 1000000.0)),
            static_cast<LONG>(std::clamp((y - view_y_) * zoom_, -1000000.0, 1000000.0))};
    }

    RECT Editor::screen_rect(const Rect& rectangle) const
    {
        const POINT top = screen_point(rectangle.x, rectangle.y);
        const POINT bottom = screen_point(rectangle.x + rectangle.width, rectangle.y + rectangle.height);
        return {top.x, top.y, bottom.x, bottom.y};
    }

    void Editor::draw_item(HDC dc, const DrawItem& item)
    {
        const bool selected = selection_ && same_entry(*selection_, item.entry);
        const Rect rectangle = selected && dragging_ ? drag_preview_ : item.rectangle;
        RECT screen = screen_rect(rectangle);
        const auto value = [&item](const std::string& key) -> std::string {
            const std::map<std::string, std::string>::const_iterator found = item.values.find(key);
            return found == item.values.end() ? "" : found->second;
        };
        if (item.entry.kind == EntryKind::bounds)
        {
            if (!show_bounds_ && !selected) return;
            const COLORREF colours[] = {RGB(239, 108, 113), RGB(131, 190, 236), RGB(204, 151, 246), RGB(240, 197, 95)};
            outline(dc, screen, colours[item.entry.index % 4], 1, PS_DASH);
            SetTextColor(dc, colours[item.entry.index % 4]);
            const std::wstring label = wide(item.entry.label);
            TextOutW(dc, screen.left + 5, screen.top + 4, label.c_str(), static_cast<int>(label.size()));
        }
        else if (is_spawn(item.entry))
        {
            screen = screen_rect(Rect{rectangle.x, rectangle.y, 52.0, 120.0});
            screen.right = std::max(screen.right, screen.left + 18);
            screen.bottom = std::max(screen.bottom, screen.top + 24);
            const COLORREF colour = item.entry.kind == EntryKind::spawn_a ? RGB(70, 163, 250) : RGB(255, 158, 75);
            outline(dc, screen, colour, 2);
            line(dc, screen.left - 5, screen.top, screen.left + 5, screen.top, colour, 2);
            line(dc, screen.left, screen.top - 5, screen.left, screen.top + 5, colour, 2);
            const std::wstring label = (item.entry.kind == EntryKind::spawn_a ? L"A" : L"B") + std::to_wstring(item.entry.index + 1);
            SetTextColor(dc, colour); TextOutW(dc, screen.left + 3, screen.top - 21, label.c_str(), static_cast<int>(label.size()));
        }
        else
        {
            const bool paintable = value("type") == "StructurePaintable";
            const bool ramp = value("type") == "StructureRamp";
            COLORREF colour = paintable ? RGB(114, 133, 149) : RGB(156, 110, 77);
            if (value("collision_type") == "STRUCTURE_JUMP_THROUGH") colour = RGB(187, 155, 106);
            if (item.entry.kind == EntryKind::decoration)
            {
                outline(dc, screen, RGB(91, 109, 127), 1, PS_DOT);
            }
            else if (ramp)
            {
                const bool left = value("collision_type") == "STRUCTURE_RAMP_LEFT";
                POINT points[] = {{screen.left, screen.bottom}, {screen.right, screen.bottom},
                    {left ? screen.left : screen.right, screen.top}};
                HBRUSH brush = CreateSolidBrush(colour);
                HGDIOBJ old_brush = SelectObject(dc, brush), old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
                Polygon(dc, points, 3);
                SelectObject(dc, old_brush); SelectObject(dc, old_pen); DeleteObject(brush);
            }
            else
            {
                fill(dc, screen, colour);
                outline(dc, screen, RGB(40, 49, 60));
                if (paintable)
                {
                    const COLORREF edge = RGB(87, 230, 204);
                    if (value("paintable_faces/left") == "true") line(dc, screen.left, screen.top, screen.left, screen.bottom, edge, 2);
                    if (value("paintable_faces/top") == "true") line(dc, screen.left, screen.top, screen.right, screen.top, edge, 2);
                    if (value("paintable_faces/right") == "true") line(dc, screen.right, screen.top, screen.right, screen.bottom, edge, 2);
                    if (value("paintable_faces/bottom") == "true") line(dc, screen.left, screen.bottom, screen.right, screen.bottom, edge, 2);
                }
            }
        }
        if (selected)
        {
            outline(dc, screen, RGB(255, 255, 255), 2);
            if (!is_spawn(item.entry))
                fill(dc, RECT{screen.right - 5, screen.bottom - 5, screen.right + 6, screen.bottom + 6}, RGB(255, 255, 255));
        }
    }

    void Editor::paint(HDC dc, const RECT& client)
    {
        fill(dc, client, RGB(28, 36, 46));
        SetBkMode(dc, TRANSPARENT);
        HGDIOBJ old_font = SelectObject(dc, font_);
        double spacing = static_cast<double>(snap_);
        while (spacing * zoom_ < 28.0) spacing *= 2.0;
        const double pixel_spacing = spacing * zoom_;
        const double first_x = -std::fmod(view_x_, spacing) * zoom_;
        const double first_y = -std::fmod(view_y_, spacing) * zoom_;
        const int columns = static_cast<int>(client.right / pixel_spacing) + 3;
        const int rows = static_cast<int>(client.bottom / pixel_spacing) + 3;
        for (int index = 0; index < columns; ++index)
        {
            const int x = static_cast<int>(first_x + index * pixel_spacing);
            line(dc, x, 0, x, client.bottom, RGB(40, 50, 62));
        }
        for (int index = 0; index < rows; ++index)
        {
            const int y = static_cast<int>(first_y + index * pixel_spacing);
            line(dc, 0, y, client.right, y, RGB(40, 50, 62));
        }
        const POINT origin = screen_point(0, 0);
        line(dc, origin.x, 0, origin.x, client.bottom, RGB(64, 77, 94));
        line(dc, 0, origin.y, client.right, origin.y, RGB(64, 77, 94));
        for (EntryKind kind : {EntryKind::decoration, EntryKind::collision, EntryKind::bounds, EntryKind::spawn_a, EntryKind::spawn_b})
            for (const DrawItem& item : draw_items_) if (item.entry.kind == kind) draw_item(dc, item);
        if (selection_)
            for (const DrawItem& item : draw_items_) if (same_entry(item.entry, *selection_)) draw_item(dc, item);
        const std::wstring legend = L"GEOMETRY VIEW    " + std::to_wstring(static_cast<int>(std::round(zoom_ * 100.0)))
            + L"%    |    Grid " + std::to_wstring(static_cast<int>(spacing));
        fill(dc, RECT{8, 8, 370, 35}, RGB(28, 36, 46));
        SetTextColor(dc, RGB(181, 197, 213)); TextOutW(dc, 16, 13, legend.c_str(), static_cast<int>(legend.size()));
        SelectObject(dc, old_font);
    }

    std::optional<Entry> Editor::hit(POINT point) const
    {
        for (EntryKind kind : {EntryKind::spawn_b, EntryKind::spawn_a, EntryKind::collision, EntryKind::decoration})
        {
            for (std::vector<DrawItem>::const_reverse_iterator item = draw_items_.rbegin(); item != draw_items_.rend(); ++item)
            {
                if (item->entry.kind != kind) continue;
                RECT rectangle = screen_rect(item->rectangle);
                if (is_spawn(item->entry))
                {
                    rectangle = screen_rect(Rect{item->rectangle.x, item->rectangle.y, 52.0, 120.0});
                    rectangle.right = std::max(rectangle.right, rectangle.left + 18);
                    rectangle.bottom = std::max(rectangle.bottom, rectangle.top + 24);
                }
                InflateRect(&rectangle, 3, 3);
                if (PtInRect(&rectangle, point)) return item->entry;
            }
        }
        return std::nullopt;
    }

    double Editor::snapped(double value) const
    {
        return (GetKeyState(VK_SHIFT) & 0x8000) ? value : std::round(value / snap_) * snap_;
    }

    void Editor::cancel_drag()
    {
        dragging_ = false; panning_ = false; resizing_ = false;
        if (GetCapture() == canvas_) ReleaseCapture();
        InvalidateRect(canvas_, nullptr, FALSE);
    }

    void Editor::nudge(int x, int y)
    {
        if (!selection_ || selection_->kind == EntryKind::level || !apply_pending()) return;
        Rect rectangle = document_.rectangle(*selection_);
        const double step = (GetKeyState(VK_SHIFT) & 0x8000) ? 1.0 : static_cast<double>(snap_);
        rectangle.x += x * step; rectangle.y += y * step;
        document_.set_rectangle(*selection_, rectangle); refresh(true);
    }

    LRESULT Editor::canvas_message(UINT message_id, WPARAM wparam, LPARAM lparam)
    {
        const POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        switch (message_id)
        {
        case WM_ERASEBKGND: return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT info{}; HDC target = BeginPaint(canvas_, &info);
            RECT client{}; GetClientRect(canvas_, &client);
            HDC buffer = CreateCompatibleDC(target);
            HBITMAP bitmap = CreateCompatibleBitmap(target, std::max(1L, client.right), std::max(1L, client.bottom));
            HGDIOBJ old = SelectObject(buffer, bitmap);
            paint(buffer, client);
            BitBlt(target, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);
            SelectObject(buffer, old); DeleteObject(bitmap); DeleteDC(buffer); EndPaint(canvas_, &info); return 0;
        }
        case WM_LBUTTONDOWN:
        {
            if (!apply_pending()) return 0;
            SetFocus(canvas_);
            resizing_ = false;
            if (selection_ && selection_->kind != EntryKind::level && !is_spawn(*selection_))
            {
                const RECT rectangle = screen_rect(document_.rectangle(*selection_));
                resizing_ = std::abs(point.x - rectangle.right) <= 9 && std::abs(point.y - rectangle.bottom) <= 9;
            }
            if (!resizing_)
            {
                // Selected bounds can be moved by their outline without intercepting geometry.
                bool on_bound = false;
                if (selection_ && selection_->kind == EntryKind::bounds)
                {
                    RECT rectangle = screen_rect(document_.rectangle(*selection_));
                    RECT inner = rectangle; InflateRect(&rectangle, 5, 5); InflateRect(&inner, -5, -5);
                    on_bound = PtInRect(&rectangle, point) && !PtInRect(&inner, point);
                }
                if (!on_bound)
                {
                    const std::optional<Entry> found = hit(point);
                    if (!found) return 0;
                    select(*found);
                }
            }
            if (!selection_ || selection_->kind == EntryKind::level) return 0;
            drag_original_ = document_.rectangle(*selection_); drag_preview_ = drag_original_;
            mouse_start_ = point; dragging_ = true; SetCapture(canvas_); return 0;
        }
        case WM_MBUTTONDOWN:
            if (!apply_pending()) return 0;
            SetFocus(canvas_); cancel_drag(); panning_ = true; mouse_start_ = point; SetCapture(canvas_); return 0;
        case WM_MOUSEMOVE:
            if (dragging_)
            {
                const double dx = snapped((point.x - mouse_start_.x) / zoom_);
                const double dy = snapped((point.y - mouse_start_.y) / zoom_);
                drag_preview_ = drag_original_;
                if (resizing_)
                {
                    drag_preview_.width = std::max(1.0, drag_original_.width + dx);
                    drag_preview_.height = std::max(1.0, drag_original_.height + dy);
                }
                else { drag_preview_.x += dx; drag_preview_.y += dy; }
                InvalidateRect(canvas_, nullptr, FALSE);
            }
            else if (panning_)
            {
                view_x_ -= (point.x - mouse_start_.x) / zoom_; view_y_ -= (point.y - mouse_start_.y) / zoom_;
                mouse_start_ = point; InvalidateRect(canvas_, nullptr, FALSE);
            }
            else
            {
                std::wostringstream coordinates;
                coordinates.precision(8);
                coordinates << L"X " << view_x_ + point.x / zoom_ << L"   Y " << view_y_ + point.y / zoom_
                    << L"   |   Wheel zoom  /  Middle drag pan  /  Shift bypass snap";
                set_status(coordinates.str());
            }
            return 0;
        case WM_LBUTTONUP:
            if (dragging_ && selection_)
            {
                const Rect result = drag_preview_; cancel_drag();
                if (result.x != drag_original_.x || result.y != drag_original_.y ||
                    result.width != drag_original_.width || result.height != drag_original_.height)
                { document_.set_rectangle(*selection_, result); refresh(true); }
            }
            return 0;
        case WM_MBUTTONUP: cancel_drag(); return 0;
        case WM_CAPTURECHANGED: dragging_ = false; panning_ = false; InvalidateRect(canvas_, nullptr, FALSE); return 0;
        case WM_MOUSEWHEEL:
        {
            if (dragging_ || panning_) return 0;
            POINT local = point; ScreenToClient(canvas_, &local);
            const double x = view_x_ + local.x / zoom_, y = view_y_ + local.y / zoom_;
            zoom_ = std::clamp(zoom_ * std::pow(1.2, GET_WHEEL_DELTA_WPARAM(wparam) / 120.0), 0.005, 10.0);
            view_x_ = x - local.x / zoom_; view_y_ = y - local.y / zoom_;
            InvalidateRect(canvas_, nullptr, FALSE); return 0;
        }
        case WM_KEYDOWN:
            switch (wparam)
            {
            case VK_ESCAPE: cancel_drag(); break;
            case VK_DELETE: command(edit_delete); break;
            case VK_HOME: command(view_fit); break;
            case 'F': command(view_selection); break;
            case VK_LEFT: nudge(-1, 0); break;
            case VK_RIGHT: nudge(1, 0); break;
            case VK_UP: nudge(0, -1); break;
            case VK_DOWN: nudge(0, 1); break;
            default: break;
            }
            return 0;
        default: break;
        }
        return DefWindowProcW(canvas_, message_id, wparam, lparam);
    }

    LRESULT Editor::message(UINT message_id, WPARAM wparam, LPARAM lparam)
    {
        switch (message_id)
        {
        case WM_CREATE: create_controls(); return 0;
        case WM_SIZE: layout(); return 0;
        case WM_GETMINMAXINFO:
            reinterpret_cast<MINMAXINFO*>(lparam)->ptMinTrackSize = POINT{1150, 720}; return 0;
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

    LRESULT CALLBACK Editor::canvas_proc(HWND window, UINT message_id, WPARAM wparam, LPARAM lparam)
    {
        Editor* editor = reinterpret_cast<Editor*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message_id == WM_NCCREATE)
        {
            editor = static_cast<Editor*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
            editor->canvas_ = window; SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(editor));
        }
        if (!editor) return DefWindowProcW(window, message_id, wparam, lparam);
        try { return editor->canvas_message(message_id, wparam, lparam); }
        catch (const std::exception& exception) { editor->cancel_drag(); editor->error(exception); return 0; }
    }

    int Editor::run(int show, const std::filesystem::path& initial_path)
    {
        SetProcessDPIAware();
        INITCOMMONCONTROLSEX common{sizeof(common), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&common);
        WNDCLASSEXW type{};
        type.cbSize = sizeof(type); type.hInstance = instance_; type.lpfnWndProc = window_proc;
        type.hCursor = LoadCursorW(nullptr, IDC_ARROW); type.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        type.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1); type.lpszClassName = L"ColourWarsLevelEditor";
        if (!RegisterClassExW(&type)) throw std::runtime_error("Could not register the editor window.");
        type.lpfnWndProc = canvas_proc; type.lpszClassName = L"ColourWarsEditorCanvas"; type.hbrBackground = nullptr;
        if (!RegisterClassExW(&type)) throw std::runtime_error("Could not register the canvas.");
        if (!CreateWindowExW(WS_EX_CONTROLPARENT, L"ColourWarsLevelEditor", L"ColourWars Level Editor",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1500, 940,
            nullptr, nullptr, instance_, this)) throw std::runtime_error("Could not open the editor.");
        if (initial_path.empty()) document_.create(); else document_.load(initial_path);
        refresh(); layout(); command(view_match);
        ShowWindow(window_, show); UpdateWindow(window_);
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
        DestroyAcceleratorTable(accelerators_); DeleteObject(font_);
        return static_cast<int>(message_data.wParam);
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    try
    {
        int count = 0; wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
        const std::filesystem::path path = arguments && count > 1 ? std::filesystem::path(arguments[1]) : std::filesystem::path{};
        if (arguments) LocalFree(arguments);
        Editor editor(instance);
        return editor.run(show, path);
    }
    catch (const std::exception& error)
    {
        MessageBoxW(nullptr, wide(error.what()).c_str(), L"ColourWars Level Editor", MB_OK | MB_ICONERROR);
        return 1;
    }
}
