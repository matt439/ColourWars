// WM_DPICHANGED carries a RECT pointer. Drive the production window in this
// process, never pass a cross-process pointer from a shell/native-message probe.
#include "editor/editor.h"
#include "editor/window_dpi.h"
#include "editor/window_layout.h"
#include "editor/win32_text.h"

#include <commctrl.h>
#include <shellscalingapi.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace colourwars_editor
{
    namespace
    {
        void require(bool condition, const char* message)
        {
            if (!condition) { throw std::runtime_error(message); }
        }

        bool same_rectangle(const RECT& first, const RECT& second)
        {
            return first.left == second.left && first.top == second.top &&
                first.right == second.right && first.bottom == second.bottom;
        }

        RECT child_rectangle(HWND parent, HWND child)
        {
            RECT rectangle{};
            require(GetWindowRect(child, &rectangle) != 0, "Could not inspect an editor child rectangle");
            MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rectangle), 2);
            return rectangle;
        }

        bool contained(const RECT& outer, const RECT& inner)
        {
            return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right &&
                inner.bottom <= outer.bottom && inner.right > inner.left && inner.bottom > inner.top;
        }

        bool overlaps(const RECT& first, const RECT& second)
        {
            return first.left < second.right && first.right > second.left &&
                first.top < second.bottom && first.bottom > second.top;
        }

        void paint_canvas(Editor& editor, HWND canvas, void (*paint)(Editor&, HDC, const RECT&))
        {
            RECT client{}; GetClientRect(canvas, &client);
            HDC dc = CreateCompatibleDC(nullptr);
            require(dc != nullptr, "Could not create the canvas probe DC");
            BITMAPINFO information{};
            information.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            information.bmiHeader.biWidth = std::max(1L, client.right);
            information.bmiHeader.biHeight = -std::max(1L, client.bottom);
            information.bmiHeader.biPlanes = 1;
            information.bmiHeader.biBitCount = 32;
            information.bmiHeader.biCompression = BI_RGB;
            void* pixels = nullptr;
            HBITMAP bitmap = CreateDIBSection(dc, &information, DIB_RGB_COLORS, &pixels, nullptr, 0);
            if (!bitmap) { DeleteDC(dc); throw std::runtime_error("Could not create the canvas probe bitmap"); }
            HGDIOBJ previous = SelectObject(dc, bitmap);
            try { paint(editor, dc, client); }
            catch (...)
            {
                SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc); throw;
            }
            SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        }
    }

    struct EditorDpiProbe
    {
        static void check_mechanisms(Editor& editor, const std::filesystem::path& scratch);
        static void check_metadata(Editor& editor, const std::filesystem::path& scratch);
        static void check_gestures(Editor& editor, const std::filesystem::path& scratch);
        static void check_ramp_hits(Editor& editor, const std::filesystem::path& scratch);
        static void run_gestures(const std::filesystem::path& scratch);

        static void check_controls(Editor& editor)
        {
            RECT client{}; GetClientRect(editor.window_, &client);
            std::vector<HWND> controls(editor.toolbar_.begin(), editor.toolbar_.end());
            for (HWND child : { editor.palette_, editor.grid_, editor.object_heading_, editor.inspector_heading_,
                editor.object_list_, editor.canvas_, editor.property_list_, editor.property_label_, editor.value_edit_,
                editor.apply_, editor.revert_, editor.status_ }) { controls.push_back(child); }
            if ((GetWindowLongPtrW(editor.hint_, GWL_STYLE) & WS_VISIBLE) != 0) { controls.push_back(editor.hint_); }
            std::vector<RECT> rectangles;
            for (HWND child : controls)
            {
                const RECT bounds = child_rectangle(editor.window_, child);
                require(contained(client, bounds), "DPI change put a native child outside the editor client");
                for (const RECT& other : rectangles)
                { require(!overlaps(bounds, other), "DPI change overlapped native editor controls"); }
                rectangles.push_back(bounds);
                if (child != editor.canvas_)
                {
                    require(reinterpret_cast<HFONT>(SendMessageW(child, WM_GETFONT, 0, 0)) == editor.font_,
                        "A native editor control retained the previous DPI font");
                }
            }
            const RECT apply = child_rectangle(editor.window_, editor.apply_);
            const RECT edit = child_rectangle(editor.window_, editor.value_edit_);
            require(apply.bottom - apply.top == MulDiv(30, static_cast<int>(editor.dpi_), 96) &&
                edit.bottom - edit.top == MulDiv(28, static_cast<int>(editor.dpi_), 96),
                "Native property/action heights did not follow editor DPI");
            require(IsWindowEnabled(editor.apply_) && IsWindowEnabled(editor.revert_),
                "DPI layout made the selected property's actions unavailable");
            LOGFONTW font{};
            require(GetObjectW(editor.font_, static_cast<int>(sizeof(font)), &font) == static_cast<int>(sizeof(font)) &&
                font.lfHeight == -MulDiv(16, static_cast<int>(editor.dpi_), 96),
                "Native editor font did not follow the 96-DPI baseline");
        }

        static void run(const std::filesystem::path& scratch)
        {
            const std::filesystem::path stage = scratch / "dpi-stage.json";
            LevelDocument seed;
            seed.save(stage);
            Editor editor(GetModuleHandleW(nullptr));
            editor.create_window(SW_HIDE, stage);
            require(!IsWindowVisible(editor.window_), "Native DPI probe unexpectedly showed its editor");
            check_metadata(editor, scratch);
            PROCESS_DPI_AWARENESS awareness = PROCESS_DPI_UNAWARE;
            require(SUCCEEDED(GetProcessDpiAwareness(nullptr, &awareness)) && awareness == PROCESS_PER_MONITOR_DPI_AWARE,
                "Editor process is not per-monitor DPI aware");
            const RECT work = editor_monitor_work_area(MonitorFromWindow(editor.window_, MONITOR_DEFAULTTONEAREST));
            RECT initial{}; GetWindowRect(editor.window_, &initial);
            require(contained(work, initial), "Initial native editor escaped its monitor's work area");

            const Entry spawn{ EntryKind::spawn_a, 0, "Team A spawn 1" };
            Rect rectangle = editor.document_.rectangle(spawn);
            const std::string saved = editor.document_.serialize();
            rectangle.x += 25.0;
            editor.document_.set_rectangle(spawn, rectangle);
            const std::string first_edit = editor.document_.serialize();
            rectangle.x += 25.0;
            editor.document_.set_rectangle(spawn, rectangle);
            const std::string second_edit = editor.document_.serialize();
            require(editor.document_.undo(), "Could not prepare a DPI fixture with redo history");
            const Rect live_rectangle = editor.document_.rectangle(spawn);
            editor.selection_ = spawn;
            editor.refresh();
            int row = -1;
            for (std::size_t index = 0; index < editor.properties_.size(); ++index)
                if (editor.properties_[index].path == "x") { row = static_cast<int>(index); }
            require(row >= 0, "Spawn fixture has no X property");
            require(editor.properties_[static_cast<std::size_t>(row)].numeric,
                "Spawn X fixture must select the inspector's numeric coordinate row");
            ListView_SetItemState(editor.property_list_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            editor.select_property(row);
            SetWindowTextW(editor.value_edit_, L"unapplied -- DPI test");
            SendMessageW(editor.value_edit_, EM_SETSEL, 3, 9);
            editor.zoom_ = 0.65; editor.view_x_ = 10.25; editor.view_y_ = 100.5;
            const POINT mapped = editor.screen_point(live_rectangle.x + 10.0, live_rectangle.y + 10.0);
            const std::wstring pending = window_text(editor.value_edit_);
            const std::wstring status = window_text(editor.status_);
            const std::wstring title = window_text(editor.window_);
            const LRESULT object_selection = SendMessageW(editor.object_list_, LB_GETCURSEL, 0, 0);

            const auto unchanged = [&]
            {
                DWORD start = 0, end = 0;
                SendMessageW(editor.value_edit_, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
                require(editor.document_.serialize() == first_edit && editor.document_.path() == stage &&
                    editor.document_.dirty() && editor.document_.can_undo() && editor.document_.can_redo(),
                    "DPI handling changed document, saved path, dirty state or history availability");
                require(editor.selection_ && same_entry(*editor.selection_, spawn) && editor.property_index_ == row &&
                    ListView_GetNextItem(editor.property_list_, -1, LVNI_SELECTED) == row &&
                    SendMessageW(editor.object_list_, LB_GETCURSEL, 0, 0) == object_selection,
                    "DPI handling changed the selected object or property row");
                require(editor.pending() && window_text(editor.value_edit_) == pending && start == 3 && end == 9 &&
                    window_text(editor.status_) == status && window_text(editor.window_) == title,
                    "DPI handling applied/reset pending text, its selection, status or title");
                const POINT after = editor.screen_point(live_rectangle.x + 10.0, live_rectangle.y + 10.0);
                require(editor.zoom_ == 0.65 && editor.view_x_ == 10.25 && editor.view_y_ == 100.5 &&
                    after.x == mapped.x && after.y == mapped.y,
                    "DPI handling changed canvas zoom, origin or world-to-client conversion");
                const auto hit = editor.hit(after);
                require(hit && same_entry(*hit, spawn), "DPI-scaled hit testing lost the mapped spawn");
            };
            const auto transition = [&](UINT dpi, bool drag)
            {
                HFONT previous = editor.font_;
                const UINT previous_dpi = editor.dpi_;
                editor.dragging_ = drag; editor.resizing_ = drag; editor.panning_ = !drag;
                editor.drag_preview_ = { 900, 900, 500, 400 };
                const RECT suggested = initial_editor_rectangle(dpi, work);
                SendMessageW(editor.window_, WM_DPICHANGED, MAKEWPARAM(dpi, dpi), reinterpret_cast<LPARAM>(&suggested));
                if (previous_dpi != dpi)
                { require(GetObjectType(previous) == 0, "DPI transition retained its replaced owned font"); }
                RECT actual{}; GetWindowRect(editor.window_, &actual);
                require(editor.dpi_ == dpi && same_rectangle(actual, suggested) && !IsWindowVisible(editor.window_),
                    "WM_DPICHANGED failed to adopt its rectangle/DPI or showed the hidden editor");
                require(!editor.dragging_ && !editor.resizing_ && !editor.panning_,
                    "DPI handling retained a pointer gesture from the previous client geometry");
                require((dpi == previous_dpi && editor.font_ == previous) ||
                    (dpi != previous_dpi && editor.font_ != previous), "DPI transition did not replace/reuse its owned font");
                check_controls(editor);
                MINMAXINFO minimum{};
                SendMessageW(editor.window_, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&minimum));
                require(minimum.ptMinTrackSize.x == std::min(MulDiv(1150, static_cast<int>(dpi), 96), static_cast<int>(work.right - work.left)) &&
                    minimum.ptMinTrackSize.y == std::min(MulDiv(720, static_cast<int>(dpi), 96), static_cast<int>(work.bottom - work.top)),
                    "Native minimum tracking size did not scale/clamp to the monitor work area");
                unchanged();
                paint_canvas(editor, editor.canvas_, [](Editor& target, HDC dc, const RECT& client)
                { target.paint(dc, client); });
                unchanged();
            };
            for (UINT dpi : { 96u, 144u, 192u, 96u }) { transition(dpi, dpi == 144); }
            const DWORD warmed_gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
            for (int cycle = 0; cycle < 6; ++cycle)
                for (UINT dpi : { 96u, 144u, 192u, 96u }) { transition(dpi, dpi == 144); }
            const DWORD final_gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
            require(final_gdi == warmed_gdi, "Repeated DPI/font/canvas transitions leaked GDI resources");
            require(editor.document_.undo() && editor.document_.serialize() == saved && !editor.document_.dirty() &&
                editor.document_.redo() && editor.document_.serialize() == first_edit &&
                editor.document_.redo() && editor.document_.serialize() == second_edit &&
                editor.document_.undo() && editor.document_.serialize() == first_edit,
                "DPI transitions changed the complete prepared undo/redo branch");
            std::cout << "PASS native editor DPI: 28 in-process96/144/192 transitions, suggested rectangles, "
                "scaled fonts/controls/minimums, pending text+selection, view/hit conversion, full history and stable GDI "
                << warmed_gdi << " -> " << final_gdi << '\n';
            check_mechanisms(editor, scratch);
        }
    };
}

#include "scripts/acceptance/level_editor_dpi/mechanism_checks.h"
#include "scripts/acceptance/level_editor_dpi/metadata_checks.h"
#include "scripts/acceptance/level_editor_dpi/gesture_checks.h"
#include "scripts/acceptance/level_editor_dpi/ramp_hit_checks.h"

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        const bool gestures = argc == 3 && std::wstring(argv[2]) == L"--gestures";
        if (argc != 2 && !gestures)
        { throw std::runtime_error("Usage: colourwars_editor_dpi_acceptance <scratch-directory> [--gestures]"); }
        const std::filesystem::path scratch = std::filesystem::absolute(argv[1]);
        std::filesystem::create_directories(scratch);
        if (gestures) { colourwars_editor::EditorDpiProbe::run_gestures(scratch); }
        else { colourwars_editor::EditorDpiProbe::run(scratch); }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL native editor DPI: " << error.what() << '\n';
        return 1;
    }
}
