#pragma once

// Included after mechanism_checks.h. These controlled exceptions reach the
// actual private paint methods through EditorDpiProbe's existing friendship.
// They never pass through canvas_proc's modal error boundary.
#include <array>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <vector>

namespace colourwars_editor
{
    inline void EditorDpiProbe::check_paint_failure(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto private_bytes = [](const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Could not read the paint-failure private source");
            const std::string result{ std::istreambuf_iterator<char>(input), {} };
            require(!input.bad(), "Could not read the complete paint-failure private source");
            return result;
        };
        const auto profile = []() -> std::optional<std::wstring>
        {
            // Read only this process's environment key, never its directory.
            SetLastError(ERROR_SUCCESS);
            const DWORD needed = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
            if (needed == 0)
            {
                const DWORD error = GetLastError();
                require(error == ERROR_SUCCESS || error == ERROR_ENVVAR_NOT_FOUND,
                    "Could not inspect the paint-failure caller's profile key");
                if (error == ERROR_ENVVAR_NOT_FOUND) return std::nullopt;
                return std::wstring{};
            }
            std::vector<wchar_t> value(needed);
            const DWORD copied = GetEnvironmentVariableW(L"LOCALAPPDATA", value.data(), needed);
            require(copied > 0 && copied < needed,
                "The paint-failure caller's profile key changed while it was read");
            return std::wstring(value.data(), copied);
        };

        require(IsWindow(editor.window_) && IsWindow(editor.canvas_) && !IsWindowVisible(editor.window_),
            "Paint-failure qualification needs its existing hidden private editor");
        const auto source_path = scratch / "dpi-stage.json";
        const std::string source = private_bytes(source_path);
        const std::string document = editor.document_.serialize();
        const auto document_path = editor.document_.path();
        const bool dirty = editor.document_.dirty(), undo = editor.document_.can_undo(), redo = editor.document_.can_redo();
        const auto history = native_history(editor.document_);
        const auto selection = editor.selection_;
        const auto property_path = [&]() -> std::optional<std::string>
        {
            if (editor.property_index_ >= 0 && static_cast<std::size_t>(editor.property_index_) < editor.properties_.size())
                return editor.properties_[static_cast<std::size_t>(editor.property_index_)].path;
            return std::nullopt;
        };
        const int property_index = editor.property_index_;
        const auto selected_property = property_path();
        const int property_row = ListView_GetNextItem(editor.property_list_, -1, LVNI_SELECTED);
        const LRESULT object_row = SendMessageW(editor.object_list_, LB_GETCURSEL, 0, 0);
        const std::wstring text = window_text(editor.value_edit_), status = window_text(editor.status_);
        const std::wstring title = window_text(editor.window_);
        DWORD text_start = 0, text_end = 0;
        SendMessageW(editor.value_edit_, EM_GETSEL, reinterpret_cast<WPARAM>(&text_start), reinterpret_cast<LPARAM>(&text_end));
        const bool pending = editor.pending();
        const double zoom = editor.zoom_, view_x = editor.view_x_, view_y = editor.view_y_;
        const int snap = editor.snap_;
        const LRESULT grid = SendMessageW(editor.grid_, CB_GETCURSEL, 0, 0);
        const bool show_bounds = editor.show_bounds_, show_blockout = editor.show_blockout_;
        const bool dragging = editor.dragging_, resizing = editor.resizing_, panning = editor.panning_;
        const Rect drag_original = editor.drag_original_, drag_preview = editor.drag_preview_;
        const POINT mouse_start = editor.mouse_start_;
        const HWND window = editor.window_, canvas = editor.canvas_, focus = GetFocus(), capture = GetCapture();
        const HWND active = GetActiveWindow();
        const HFONT font = editor.font_;
        const UINT dpi = editor.dpi_;
        RECT window_rectangle{}, canvas_rectangle{};
        require(GetWindowRect(window, &window_rectangle) && GetWindowRect(canvas, &canvas_rectangle),
            "Could not retain paint-failure caller rectangles");
        std::array<BYTE, 256> keys{};
        require(GetKeyboardState(keys.data()) != FALSE, "Could not retain paint-failure caller thread keys");
        const auto caller_profile = profile();
        const DrawItem* draw_data = editor.draw_items_.data();
        const std::size_t draw_size = editor.draw_items_.size(), draw_capacity = editor.draw_items_.capacity();

        const auto same_rect = [](const Rect& first, const Rect& second)
        {
            return first.x == second.x && first.y == second.y && first.width == second.width && first.height == second.height;
        };
        struct CallerState
        {
            bool source = false, document = false, history = false, selection = false, property = false;
            bool view_grid = false, gestures = false, shell = false, keys = false, focus_capture = false;
            bool draw_restore = false, profile = false;
            bool good() const
            {
                return source && document && history && selection && property && view_grid && gestures &&
                    shell && keys && focus_capture && draw_restore && profile;
            }
        };
        const auto caller_state = [&]() -> CallerState
        {
            DWORD start = 0, end = 0;
            SendMessageW(editor.value_edit_, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
            RECT current_window{}, current_canvas{};
            const BOOL have_window = GetWindowRect(editor.window_, &current_window);
            const BOOL have_canvas = GetWindowRect(editor.canvas_, &current_canvas);
            std::array<BYTE, 256> current_keys{};
            const BOOL have_keys = GetKeyboardState(current_keys.data());
            return {
                private_bytes(source_path) == source,
                editor.document_.serialize() == document && editor.document_.path() == document_path &&
                    editor.document_.dirty() == dirty,
                editor.document_.can_undo() == undo && editor.document_.can_redo() == redo &&
                    native_history(editor.document_) == history,
                editor.selection_.has_value() == selection.has_value() &&
                    (!selection || (same_entry(*editor.selection_, *selection) && editor.selection_->label == selection->label)) &&
                    SendMessageW(editor.object_list_, LB_GETCURSEL, 0, 0) == object_row,
                editor.property_index_ == property_index && property_path() == selected_property &&
                    ListView_GetNextItem(editor.property_list_, -1, LVNI_SELECTED) == property_row &&
                    window_text(editor.value_edit_) == text && start == text_start && end == text_end &&
                    editor.pending() == pending && window_text(editor.status_) == status && window_text(editor.window_) == title,
                editor.zoom_ == zoom && editor.view_x_ == view_x && editor.view_y_ == view_y && editor.snap_ == snap &&
                    SendMessageW(editor.grid_, CB_GETCURSEL, 0, 0) == grid && editor.show_bounds_ == show_bounds &&
                    editor.show_blockout_ == show_blockout,
                editor.dragging_ == dragging && editor.resizing_ == resizing && editor.panning_ == panning &&
                    same_rect(editor.drag_original_, drag_original) && same_rect(editor.drag_preview_, drag_preview) &&
                    editor.mouse_start_.x == mouse_start.x && editor.mouse_start_.y == mouse_start.y,
                editor.window_ == window && editor.canvas_ == canvas && editor.font_ == font && editor.dpi_ == dpi &&
                    have_window && have_canvas && same_rectangle(current_window, window_rectangle) &&
                    same_rectangle(current_canvas, canvas_rectangle) && IsWindow(window) && IsWindow(canvas) &&
                    !IsWindowVisible(window),
                have_keys && current_keys == keys,
                GetFocus() == focus && GetCapture() == capture && GetActiveWindow() == active,
                editor.draw_items_.data() == draw_data && editor.draw_items_.size() == draw_size &&
                    editor.draw_items_.capacity() == draw_capacity,
                profile() == caller_profile
            };
        };
        const auto write_state = [](std::ofstream& output, const CallerState& state)
        {
            output << ',' << state.source << ',' << state.document << ',' << state.history << ',' << state.selection << ','
                << state.property << ',' << state.view_grid << ',' << state.gestures << ',' << state.shell << ',' << state.keys
                << ',' << state.focus_capture << ',' << state.draw_restore << ',' << state.profile << '\n';
        };
        const char* state_columns = ",source,document,history,selection,property,view_grid,gestures,shell,keys,focus_capture,draw_restore,profile\n";

        // Prepare the malformed in-memory label before swapping caller state.
        // No malformed document is loaded or written and no allocation failure
        // is injected. wide() rejects this byte at the existing draw boundary.
        DrawItem invalid;
        invalid.entry = { EntryKind::bounds, 0, std::string(1, static_cast<char>(0xff)) };
        invalid.rectangle = { 10, 20, 100, 80 };
        std::vector<DrawItem> failing_draw_items{ invalid };
        struct DrawScope
        {
            DrawScope(std::vector<DrawItem>& target_value, std::vector<DrawItem>& replacement_value, bool& bounds_value) noexcept
                : target(target_value), replacement(replacement_value), bounds(bounds_value), previous(bounds_value)
            { target.swap(replacement); bounds = true; }
            ~DrawScope() { target.swap(replacement); bounds = previous; }
            DrawScope(const DrawScope&) = delete;
            DrawScope& operator=(const DrawScope&) = delete;
            std::vector<DrawItem>& target;
            std::vector<DrawItem>& replacement;
            bool& bounds;
            bool previous;
        };

        std::ofstream font_evidence(scratch / "paint-failure-font.csv", std::ios::trunc);
        std::ofstream buffer_evidence(scratch / "paint-failure-buffer.csv", std::ios::trunc);
        require(font_evidence.good() && buffer_evidence.good(), "Could not create paint-failure evidence");
        font_evidence << "exact_exception,font_before,font_after,font_restored,font_repair_ok,live_font_record_bytes" << state_columns;
        buffer_evidence << "phase,trip,exact_exception,raw_gdi,after_thread_flush_gdi,flush_ok,warmed_gdi" << state_columns;

        bool font_exception = false, font_restored = false, font_repair_ok = false;
        UINT_PTR before_font = 0, after_font = 0;
        {
            MechanismBitmap bitmap(256, 256);
            const HGDIOBJ control_font = GetStockObject(ANSI_FIXED_FONT);
            require(control_font && control_font != editor.font_, "Paint-failure font control is not distinct from the editor font");
            const HGDIOBJ original_font = SelectObject(bitmap.dc(), control_font);
            require(original_font && original_font != HGDI_ERROR, "Could not select the paint-failure caller's control font");
            struct FontRepair
            {
                HDC dc;
                HGDIOBJ previous;
                ~FontRepair() { SelectObject(dc, previous); }
            } repair{ bitmap.dc(), original_font };
            before_font = reinterpret_cast<UINT_PTR>(GetCurrentObject(bitmap.dc(), OBJ_FONT));
            {
                DrawScope restore(editor.draw_items_, failing_draw_items, editor.show_bounds_);
                try { editor.paint(bitmap.dc(), bitmap.rectangle()); }
                catch (const std::runtime_error& exception)
                { font_exception = std::strcmp(exception.what(), "Text is not valid UTF-8.") == 0; }
            }
            after_font = reinterpret_cast<UINT_PTR>(GetCurrentObject(bitmap.dc(), OBJ_FONT));
            font_restored = before_font == reinterpret_cast<UINT_PTR>(control_font) && after_font == before_font;
            // Observe first, then repair the private DC even on the old broken
            // implementation. The repair cannot turn font_restored into true.
            const HGDIOBJ replaced = SelectObject(bitmap.dc(), control_font);
            font_repair_ok = replaced && replaced != HGDI_ERROR && GetCurrentObject(bitmap.dc(), OBJ_FONT) == control_font;
        }
        LOGFONTW live_font{};
        const int live_font_bytes = GetObjectW(editor.font_, static_cast<int>(sizeof(live_font)), &live_font);
        const CallerState font_state = caller_state();
        font_evidence << font_exception << ',' << before_font << ',' << after_font << ',' << font_restored << ','
            << font_repair_ok << ',' << live_font_bytes;
        write_state(font_evidence, font_state);
        font_evidence.flush();

        // One valid warm paint, then a predetermined three exceptional calls.
        // The original control tree, DPI, fonts and document stay unchanged
        // inside these count intervals. No extra warm-ups or retries occur.
        editor.canvas_message(WM_PAINT, 0, 0);
        const DWORD warm_raw = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        const BOOL warm_flush = GdiFlush();
        const DWORD warmed_gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        const CallerState warm_state = caller_state();
        buffer_evidence << "warm,0,-1," << warm_raw << ',' << warmed_gdi << ',' << warm_flush << ',' << warmed_gdi;
        write_state(buffer_evidence, warm_state);
        buffer_evidence.flush();
        bool all_exceptions = true, all_flushes = warm_flush != FALSE, all_counts = warmed_gdi != 0;
        bool all_states = font_state.good() && warm_state.good();
        for (int trip = 1; trip <= 3; ++trip)
        {
            bool exact_exception = false;
            {
                DrawScope restore(editor.draw_items_, failing_draw_items, editor.show_bounds_);
                try { editor.canvas_message(WM_PAINT, 0, 0); }
                catch (const std::runtime_error& exception)
                { exact_exception = std::strcmp(exception.what(), "Text is not valid UTF-8.") == 0; }
            }
            const DWORD raw = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
            const BOOL flushed = GdiFlush();
            const DWORD settled = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
            const CallerState state = caller_state();
            buffer_evidence << "exception," << trip << ',' << exact_exception << ',' << raw << ',' << settled << ','
                << flushed << ',' << warmed_gdi;
            write_state(buffer_evidence, state);
            buffer_evidence.flush();
            all_exceptions = all_exceptions && exact_exception;
            all_flushes = all_flushes && flushed != FALSE;
            all_counts = all_counts && settled == warmed_gdi;
            all_states = all_states && state.good();
        }
        editor.canvas_message(WM_PAINT, 0, 0);
        const DWORD recovery_raw = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        const BOOL recovery_flush = GdiFlush();
        const DWORD recovery_gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        const CallerState recovery_state = caller_state();
        buffer_evidence << "recovery,0,-1," << recovery_raw << ',' << recovery_gdi << ',' << recovery_flush << ',' << warmed_gdi;
        write_state(buffer_evidence, recovery_state);
        buffer_evidence.flush();

        // Both observations and all fixed trips are retained before asserting
        // either ownership regression. Counts alone do not prove EndPaint was
        // paired; the production scope guard must also be reviewed for that.
        require(font_evidence.good() && buffer_evidence.good(), "Could not retain complete paint-failure evidence");
        require(font_exception && all_exceptions, "Paint-failure fixture did not reach its exact controlled UTF-8 exception");
        require(all_states && recovery_state.good(), "Paint-failure drawing changed its private caller state or source");
        require(font_repair_ok, "Paint-failure fixture could not repair its private bitmap's selected font");
        require(live_font_bytes == static_cast<int>(sizeof(live_font)) &&
            live_font.lfHeight == -MulDiv(16, static_cast<int>(dpi), 96),
            "Paint-failure drawing damaged the editor's live owned font record");
        require(font_restored, "Exceptional editor paint did not restore the caller's selected font");
        require(all_flushes && recovery_flush != FALSE, "Paint-failure GDI completion barrier reported a failed batch");
        require(all_counts && recovery_gdi == warmed_gdi,
            "Exceptional buffered editor paints retained GDI resources after unwinding");
        std::cout << "PASS native paint failure: exact UTF-8 exception, borrowed font restoration, 3 fixed buffered failures, "
            "successful recovery and stable post-thread-flush GDI " << warmed_gdi << " -> " << recovery_gdi
            << ", exact private caller/source/history restoration\n";
    }
}
