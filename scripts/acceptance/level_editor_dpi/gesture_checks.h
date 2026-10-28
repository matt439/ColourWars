#pragma once

// Included once after EditorDpiProbe; messages enter the real hidden canvas.
#include <fstream>
#include <iterator>

namespace colourwars_editor
{
    namespace
    {
        bool same_gesture_rectangle(const Rect& first, const Rect& second)
        {
            return first.x == second.x && first.y == second.y &&
                first.width == second.width && first.height == second.height;
        }

        class GestureKeyboardState
        {
        public:
            GestureKeyboardState()
            {
                require(GetKeyboardState(original_.data()) != 0, "Could not retain the probe thread's key state");
            }
            ~GestureKeyboardState() { SetKeyboardState(original_.data()); }
            void shift(bool pressed)
            {
                auto state = original_;
                state[VK_SHIFT] = state[VK_LSHIFT] = static_cast<BYTE>(pressed ? 0x80 : 0);
                state[VK_RSHIFT] = 0;
                require(SetKeyboardState(state.data()) != 0 &&
                    ((GetKeyState(VK_SHIFT) & 0x8000) != 0) == pressed,
                    "Could not establish the probe thread's local Shift state");
            }
            void restore()
            {
                require(SetKeyboardState(original_.data()) != 0, "Could not restore the probe thread's key state");
                std::array<BYTE, 256> restored{};
                require(GetKeyboardState(restored.data()) != 0 && restored == original_,
                    "Canvas qualification did not restore the probe thread's complete key state");
            }
        private:
            std::array<BYTE, 256> original_{};
        };
    }

    inline void EditorDpiProbe::check_gestures(Editor& editor, const std::filesystem::path& scratch)
    {
        const std::filesystem::path original_path = editor.document_.path();
        const auto original_selection = editor.selection_;
        const double original_zoom = editor.zoom_, original_x = editor.view_x_, original_y = editor.view_y_;
        const int original_snap = editor.snap_;
        const HWND original_focus = GetFocus();
        const std::filesystem::path stage = scratch / "canvas-gestures.json";
        LevelDocument seed;
        const Rect original{ 80, 80, 120, 80 };
        const Entry object = seed.add_object("StructurePaintable", original);
        seed.save(stage);
        const auto read_stage = [&]
        {
            std::ifstream input(stage, std::ios::binary);
            require(input.good(), "Could not read the private canvas gesture stage");
            return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        };
        const std::string disk = read_stage();
        GestureKeyboardState keyboard;
        std::string baseline;
        const auto prepare = [&]
        {
            editor.cancel_drag();
            editor.document_.load(stage);
            editor.selection_ = object;
            editor.refresh();
            editor.zoom_ = 1.0; editor.view_x_ = 0.0; editor.view_y_ = 0.0; editor.snap_ = 25;
            keyboard.shift(false);
            baseline = editor.document_.serialize();
            require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
                !editor.document_.can_redo(), "Canvas gesture fixture did not begin at its saved baseline");
        };
        const auto pointer = [&](UINT message, POINT point, WPARAM buttons = 0)
        {
            SendMessageW(editor.canvas_, message, buttons, MAKELPARAM(point.x, point.y));
        };
        const auto selected = [&]
        {
            require(editor.selection_ && same_entry(*editor.selection_, object) &&
                editor.document_.path() == stage && !IsWindowVisible(editor.window_) && read_stage() == disk,
                "Canvas input changed selection, saved path/file or hidden-window state");
        };
        const auto begin = [&](bool resize)
        {
            const RECT rectangle = editor.screen_rect(original);
            const POINT start = resize ? POINT{ rectangle.right, rectangle.bottom }
                : POINT{ rectangle.left + 20, rectangle.top + 20 };
            pointer(WM_LBUTTONDOWN, start, MK_LBUTTON);
            require(editor.dragging_ && editor.resizing_ == resize && !editor.panning_ &&
                GetCapture() == editor.canvas_, "Actual canvas button-down did not start its owned gesture");
            pointer(WM_MOUSEMOVE, POINT{ start.x + 75, start.y + 50 }, MK_LBUTTON);
            Rect preview = original;
            if (resize) { preview.width += 75; preview.height += 50; }
            else { preview.x += 75; preview.y += 50; }
            require(same_gesture_rectangle(editor.drag_preview_, preview) &&
                editor.document_.serialize() == baseline && !editor.document_.dirty(),
                "Canvas move did not retain a distinct, uncommitted snapped preview");
            return POINT{ start.x + 75, start.y + 50 };
        };
        const auto one_edit = [&](const Rect& expected)
        {
            const std::string after = editor.document_.serialize();
            require(same_gesture_rectangle(editor.document_.rectangle(object), expected) &&
                editor.document_.dirty() && editor.document_.can_undo() && !editor.document_.can_redo(),
                "Canvas edit did not commit its expected rectangle and history branch");
            require(editor.document_.undo() && editor.document_.serialize() == baseline &&
                !editor.document_.dirty() && !editor.document_.can_undo() && editor.document_.can_redo(),
                "Canvas input introduced an extra edit or lost its exact saved undo state");
            require(editor.document_.redo() && editor.document_.serialize() == after &&
                editor.document_.dirty() && editor.document_.can_undo() && !editor.document_.can_redo(),
                "Canvas input lost its exact single-edit redo state");
            selected();
        };

        // First regression: old source edits on the arrow, then overwrites that
        // edit with the distinct stale preview when the mouse is released.
        for (bool resize : { false, true })
        {
            prepare();
            const POINT drop = begin(resize);
            const WPARAM arrow = resize ? VK_UP : VK_RIGHT;
            SendMessageW(editor.canvas_, WM_KEYDOWN, arrow, 0);
            Rect expected = original;
            if (resize) { expected.y -= 25; } else { expected.x += 25; }
            require(same_gesture_rectangle(editor.document_.rectangle(object), expected),
                "Arrow input did not apply its nudge before the late mouse release");
            const std::string nudged = editor.document_.serialize();
            const bool ended = !editor.dragging_ && !editor.resizing_ && !editor.panning_ &&
                GetCapture() != editor.canvas_;
            pointer(WM_LBUTTONUP, drop);
            require(editor.document_.serialize() == nudged &&
                same_gesture_rectangle(editor.document_.rectangle(object), expected),
                "Late mouse release overwrote the arrow nudge with a stale canvas preview");
            require(ended, "Arrow input retained a canvas gesture or its mouse capture");
            one_edit(expected);
        }

        prepare();
        pointer(WM_MBUTTONDOWN, POINT{ 100, 100 }, MK_MBUTTON);
        pointer(WM_MOUSEMOVE, POINT{ 150, 125 }, MK_MBUTTON);
        require(editor.panning_ && GetCapture() == editor.canvas_ &&
            editor.view_x_ == -50 && editor.view_y_ == -25, "Actual middle drag did not pan the canvas");
        SendMessageW(editor.canvas_, WM_KEYDOWN, VK_LEFT, 0);
        const std::string panned_nudge = editor.document_.serialize();
        require(!editor.panning_ && !editor.dragging_ && !editor.resizing_ && GetCapture() != editor.canvas_,
            "Arrow input retained the active canvas pan/capture");
        pointer(WM_MOUSEMOVE, POINT{ 200, 175 }, MK_MBUTTON);
        pointer(WM_MBUTTONUP, POINT{ 200, 175 });
        require(editor.view_x_ == -50 && editor.view_y_ == -25 && editor.document_.serialize() == panned_nudge,
            "Late middle-drag input changed the view or added an edit after a nudge");
        Rect expected = original; expected.x -= 25;
        one_edit(expected);

        for (bool resize : { false, true })
        {
            prepare();
            const POINT drop = begin(resize);
            pointer(WM_LBUTTONUP, drop);
            expected = original;
            if (resize) { expected.width += 75; expected.height += 50; }
            else { expected.x += 75; expected.y += 50; }
            require(!editor.dragging_ && !editor.resizing_ && !editor.panning_ && GetCapture() != editor.canvas_,
                "Ordinary canvas drop did not end its gesture/capture");
            one_edit(expected);
        }

        for (bool fine : { false, true })
        {
            prepare(); keyboard.shift(fine);
            SendMessageW(editor.canvas_, WM_KEYDOWN, VK_DOWN, 0);
            expected = original; expected.y += fine ? 1.0 : 25.0;
            one_edit(expected);
        }

        // Apply the pending inspector coordinate before the arrow, retaining
        // both actual history entries while discarding only the pointer preview.
        prepare();
        const POINT drop = begin(false);
        int x_row = -1;
        for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            if (editor.properties_[index].path == "rectangle/x") { x_row = static_cast<int>(index); }
        require(x_row >= 0, "Canvas fixture lacks its numeric X inspector row");
        editor.select_property(x_row);
        SetWindowTextW(editor.value_edit_, L"105");
        require(editor.pending(), "Canvas fixture did not establish a valid pending inspector value");
        SendMessageW(editor.canvas_, WM_KEYDOWN, VK_RIGHT, 0);
        expected = original; expected.x = 130;
        const std::string pending_nudge = editor.document_.serialize();
        pointer(WM_LBUTTONUP, drop);
        require(editor.document_.serialize() == pending_nudge && !editor.pending() &&
            same_gesture_rectangle(editor.document_.rectangle(object), expected),
            "Arrow input lost pending-property admission, its nudge or the late-release barrier");
        require(editor.document_.undo(), "Pending-property nudge lost its first undo");
        expected.x = 105;
        const std::string pending_applied = editor.document_.serialize();
        require(same_gesture_rectangle(editor.document_.rectangle(object), expected) && editor.document_.can_undo() &&
            editor.document_.undo() && editor.document_.serialize() == baseline && !editor.document_.can_undo() &&
            !editor.document_.dirty(), "Pending-property input did not retain exactly its two edit states");
        require(editor.document_.redo() && editor.document_.serialize() == pending_applied &&
            editor.document_.redo() && editor.document_.serialize() == pending_nudge && !editor.document_.can_redo(),
            "Pending-property input lost its exact two-step redo branch");
        selected();

        keyboard.restore();
        editor.cancel_drag();
        editor.document_.load(original_path);
        editor.selection_ = original_selection;
        editor.refresh();
        editor.zoom_ = original_zoom; editor.view_x_ = original_x; editor.view_y_ = original_y;
        editor.snap_ = original_snap;
        SetFocus(original_focus);
        require(GetFocus() == original_focus, "Canvas qualification did not restore the original thread focus");
        std::cout << "PASS native canvas gestures: move/resize/pan interruption, harmless late release, "
            "ordinary drops, snap/Shift nudges, pending property, exact history and private source preservation\n";
    }

    inline void EditorDpiProbe::run_gestures(const std::filesystem::path& scratch)
    {
        // Canvas SetFocus activates its hidden parent. Keep that thread state
        // within this child process, apart from the aggregate DPI/GDI oracle.
        const std::filesystem::path stage = scratch / "gesture-shell.json";
        LevelDocument seed;
        seed.save(stage);
        Editor editor(GetModuleHandleW(nullptr));
        editor.create_window(SW_HIDE, stage);
        require(!IsWindowVisible(editor.window_), "Canvas gesture probe unexpectedly showed its editor");
        check_overlap_selection(editor, scratch);
        check_button_chords(editor, scratch);
        check_empty_key_properties(editor, scratch);
        check_gestures(editor, scratch);
        check_ramp_hits(editor, scratch);
    }
}
