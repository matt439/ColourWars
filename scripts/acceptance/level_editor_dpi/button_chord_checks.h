#pragma once

// Included after gesture_checks.h: reuse its private thread-key-state and
// rectangle helpers. All messages enter the real hidden canvas in --gestures.
namespace colourwars_editor
{
    inline void EditorDpiProbe::check_button_chords(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto original_path = editor.document_.path();
        const std::string original_document = editor.document_.serialize();
        const auto original_selection = editor.selection_;
        const double original_zoom = editor.zoom_, original_x = editor.view_x_, original_y = editor.view_y_;
        const int original_snap = editor.snap_;
        const HWND original_focus = GetFocus();
        require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
            !editor.document_.can_redo() && !editor.dragging_ && !editor.resizing_ && !editor.panning_ &&
            GetCapture() == nullptr,
            "Button chord qualification needs a clean private caller shell without capture");
        const auto stage = scratch / "button-chords.json";
        LevelDocument seed;
        const Rect original{ 80, 80, 120, 80 };
        const Entry object = seed.add_object("StructurePaintable", original);
        seed.save(stage);
        const auto bytes = [&]
        {
            std::ifstream input(stage, std::ios::binary);
            require(input.good(), "Could not read the private button chord stage");
            const std::string result{ std::istreambuf_iterator<char>(input), {} };
            require(!input.bad(), "Could not read the complete private button chord stage");
            return result;
        };
        const std::string disk = bytes();
        GestureKeyboardState keyboard;
        std::string baseline;
        std::size_t cases = 0;
        const auto pointer = [&](UINT message, POINT point, WPARAM buttons = 0)
        { SendMessageW(editor.canvas_, message, buttons, MAKELPARAM(point.x, point.y)); };
        const auto prepare = [&]
        {
            editor.cancel_drag();
            editor.document_.load(stage);
            editor.selection_ = object; editor.refresh();
            editor.zoom_ = 1; editor.view_x_ = editor.view_y_ = 0; editor.snap_ = 25;
            keyboard.shift(false);
            baseline = editor.document_.serialize();
            require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
                !editor.document_.can_redo(), "Button chord fixture did not begin at its clean saved state");
        };
        const auto preserved = [&]
        {
            require(editor.selection_ && same_entry(*editor.selection_, object) &&
                editor.document_.path() == stage && bytes() == disk && !IsWindowVisible(editor.window_),
                "Button chord changed its selection, private saved path/bytes or hidden boundary");
        };
        const auto idle = [&]
        {
            require(!editor.dragging_ && !editor.resizing_ && !editor.panning_ && GetCapture() == nullptr,
                "Button chord retained an ended gesture or its owned capture");
        };
        const auto history = [&](const std::vector<std::string>& states)
        {
            require(editor.document_.serialize() == states.back() &&
                editor.document_.dirty() == (states.size() > 1) &&
                editor.document_.can_undo() == (states.size() > 1) && !editor.document_.can_redo(),
                "Button chord did not retain its expected complete edit/history state");
            preserved();
            for (std::size_t index = states.size() - 1; index > 0; --index)
            {
                require(editor.document_.undo() && editor.document_.serialize() == states[index - 1] &&
                    editor.document_.dirty() == (index > 1) && editor.document_.can_undo() == (index > 1) &&
                    editor.document_.can_redo(), "Button chord lost an exact undo state or added a hidden edit");
                preserved();
            }
            for (std::size_t index = 1; index < states.size(); ++index)
            {
                require(editor.document_.redo() && editor.document_.serialize() == states[index] &&
                    editor.document_.dirty() && editor.document_.can_undo() &&
                    editor.document_.can_redo() == (index + 1 < states.size()),
                    "Button chord lost an exact redo state or its history boundary");
                preserved();
            }
            ++cases;
        };
        const auto pan = [&]
        {
            pointer(WM_MBUTTONDOWN, { 300, 300 }, MK_MBUTTON);
            require(editor.panning_ && !editor.dragging_ && !editor.resizing_ &&
                GetCapture() == editor.canvas_, "Middle down did not begin an owned canvas pan");
            pointer(WM_MOUSEMOVE, { 350, 325 }, MK_MBUTTON);
            require(editor.view_x_ == -50 && editor.view_y_ == -25 &&
                editor.document_.serialize() == baseline,
                "Button chord fixture did not establish its distinct uncommitted pan");
        };
        bool owned_left_start = false;
        std::string left_start_facts;
        const auto left_start = [&](bool resize, WPARAM buttons, bool defer_start_check = false)
        {
            const RECT rectangle = editor.screen_rect(original);
            const POINT start = resize ? POINT{ rectangle.right, rectangle.bottom }
                : POINT{ rectangle.left + 20, rectangle.top + 20 };
            pointer(WM_LBUTTONDOWN, start, buttons);
            owned_left_start = editor.dragging_ && editor.resizing_ == resize && GetCapture() == editor.canvas_;
            left_start_facts = "dragging=" + std::to_string(editor.dragging_) +
                ", resizing=" + std::to_string(editor.resizing_) + ", panning=" + std::to_string(editor.panning_) +
                ", captured=" + std::to_string(GetCapture() == editor.canvas_);
            require(editor.selection_ && same_entry(*editor.selection_, object),
                "Button chord fixture selected a different object at its left target");
            if (!defer_start_check)
                require(owned_left_start, "Left down did not begin its owned move/resize preview");
            return start;
        };

        // First regression reaches the actual lost edit before requiring the
        // state exclusivity that the same faulty handoff also violates.
        for (bool resize : { false, true })
        {
            for (bool old_release_first : { true, false })
            {
                prepare(); pan();
                const POINT start = left_start(resize, MK_LBUTTON | MK_MBUTTON, true);
                const bool started = owned_left_start;
                const std::string start_facts = left_start_facts;
                const bool exclusive = !editor.panning_;
                const POINT first{ start.x + 50, start.y + 25 };
                pointer(WM_MOUSEMOVE, first, MK_LBUTTON | MK_MBUTTON);
                Rect preview = original;
                if (resize) { preview.width += 50; preview.height += 25; }
                else { preview.x += 50; preview.y += 25; }
                const bool had_preview = same_gesture_rectangle(editor.drag_preview_, preview) &&
                    editor.document_.serialize() == baseline && !editor.document_.dirty();
                bool survived_old_release = true;
                if (old_release_first)
                {
                    pointer(WM_MBUTTONUP, first, MK_LBUTTON);
                    survived_old_release = editor.dragging_ && editor.resizing_ == resize &&
                        !editor.panning_ && GetCapture() == editor.canvas_;
                    const POINT last{ start.x + 75, start.y + 50 };
                    pointer(WM_MOUSEMOVE, last, MK_LBUTTON);
                    pointer(WM_LBUTTONUP, last);
                    if (resize) { preview.width += 25; preview.height += 25; }
                    else { preview.x += 25; preview.y += 25; }
                }
                else
                {
                    pointer(WM_LBUTTONUP, first, MK_MBUTTON);
                    pointer(WM_MOUSEMOVE, { first.x + 75, first.y + 75 }, MK_MBUTTON);
                    pointer(WM_MBUTTONUP, first);
                }
                const std::string lost_drag = "Pan-to-left handoff discarded the new left-button drag. " +
                    start_facts + ", preview=" + std::to_string(had_preview);
                require(same_gesture_rectangle(editor.document_.rectangle(object), preview), lost_drag.c_str());
                require(started && had_preview && exclusive && survived_old_release &&
                    editor.view_x_ == -50 && editor.view_y_ == -25,
                    "Pan-to-left handoff retained an older gesture owner or changed its retained view");
                idle();
                const std::string committed = editor.document_.serialize();
                pointer(WM_MOUSEMOVE, { 400, 400 }, MK_LBUTTON | MK_MBUTTON);
                pointer(WM_LBUTTONUP, { 400, 400 }); pointer(WM_MBUTTONUP, { 400, 400 });
                require(editor.document_.serialize() == committed && editor.view_x_ == -50 && editor.view_y_ == -25,
                    "Late button input revived a completed move/resize or an older pan");
                history({ baseline, committed });
            }
        }

        for (bool old_release_first : { true, false })
        {
            prepare();
            const POINT start = left_start(false, MK_LBUTTON);
            pointer(WM_MOUSEMOVE, { start.x + 75, start.y + 50 }, MK_LBUTTON);
            require(same_gesture_rectangle(editor.drag_preview_, { 155, 130, 120, 80 }) &&
                editor.document_.serialize() == baseline, "Left-to-pan fixture lacks an uncommitted left preview");
            pointer(WM_MBUTTONDOWN, { 200, 200 }, MK_LBUTTON | MK_MBUTTON);
            require(editor.panning_ && !editor.dragging_ && !editor.resizing_ && GetCapture() == editor.canvas_,
                "Middle down did not discard the older left preview before taking capture");
            pointer(WM_MOUSEMOVE, { 225, 225 }, MK_LBUTTON | MK_MBUTTON);
            if (old_release_first)
            {
                pointer(WM_LBUTTONUP, { 225, 225 }, MK_MBUTTON);
                require(editor.panning_ && GetCapture() == editor.canvas_,
                    "An older left release stopped the newer pan");
                pointer(WM_MOUSEMOVE, { 250, 250 }, MK_MBUTTON);
                pointer(WM_MBUTTONUP, { 250, 250 });
            }
            else
            {
                pointer(WM_MBUTTONUP, { 225, 225 }, MK_LBUTTON);
                pointer(WM_MOUSEMOVE, { 250, 250 }, MK_LBUTTON);
                pointer(WM_LBUTTONUP, { 250, 250 });
            }
            const double expected = old_release_first ? -50.0 : -25.0;
            require(editor.view_x_ == expected && editor.view_y_ == expected &&
                editor.document_.serialize() == baseline,
                "Left-to-pan handoff committed a cancelled preview, lost pan travel or resumed an older button");
            idle(); history({ baseline });
        }

        prepare();
        editor.view_x_ = 5000;
        pointer(WM_MBUTTONDOWN, { 100, 100 }, MK_MBUTTON);
        pointer(WM_MOUSEMOVE, { 125, 125 }, MK_MBUTTON);
        require(editor.panning_ && editor.view_x_ == 4975 && editor.view_y_ == -25 &&
            !editor.hit({ 150, 150 }), "Empty-space handoff fixture still hits authored geometry");
        pointer(WM_LBUTTONDOWN, { 150, 150 }, MK_LBUTTON | MK_MBUTTON);
        idle();
        pointer(WM_MOUSEMOVE, { 200, 200 }, MK_LBUTTON | MK_MBUTTON);
        pointer(WM_MBUTTONUP, { 200, 200 }, MK_LBUTTON); pointer(WM_LBUTTONUP, { 200, 200 });
        require(editor.view_x_ == 4975 && editor.view_y_ == -25 && editor.document_.serialize() == baseline,
            "Empty-space left down retained or revived the older pan");
        idle(); history({ baseline });

        prepare(); pan();
        int height_row = -1;
        for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            if (editor.properties_[index].path == "rectangle/height") { height_row = static_cast<int>(index); }
        require(height_row >= 0, "Button chord fixture lacks its height inspector row");
        ListView_SetItemState(editor.property_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_SetItemState(editor.property_list_, height_row, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
        require(editor.property_index_ == height_row, "Could not select the valid pending chord property");
        SetWindowTextW(editor.value_edit_, L"105");
        require(editor.pending(), "Button chord fixture did not establish valid pending inspector text");
        const POINT pending_start = left_start(false, MK_LBUTTON | MK_MBUTTON);
        const std::string pending = editor.document_.serialize();
        require(!editor.pending() && same_gesture_rectangle(editor.document_.rectangle(object), { 80, 80, 120, 105 }) &&
            !editor.panning_, "Left-button handoff lost pending-property admission before taking its gesture");
        const POINT pending_drop{ pending_start.x + 50, pending_start.y + 25 };
        pointer(WM_MOUSEMOVE, pending_drop, MK_LBUTTON | MK_MBUTTON);
        pointer(WM_MBUTTONUP, pending_drop, MK_LBUTTON); pointer(WM_LBUTTONUP, pending_drop);
        require(same_gesture_rectangle(editor.document_.rectangle(object), { 130, 105, 120, 105 }) &&
            editor.view_x_ == -50 && editor.view_y_ == -25,
            "Button chord lost the admitted property or its subsequent drag");
        idle(); history({ baseline, pending, editor.document_.serialize() });

        // Ordinary owning-button drops remain controls independent of a chord.
        prepare();
        const POINT ordinary = left_start(false, MK_LBUTTON);
        pointer(WM_MOUSEMOVE, { ordinary.x + 50, ordinary.y + 25 }, MK_LBUTTON);
        pointer(WM_LBUTTONUP, { ordinary.x + 50, ordinary.y + 25 });
        require(same_gesture_rectangle(editor.document_.rectangle(object), { 130, 105, 120, 80 }) &&
            editor.view_x_ == 0 && editor.view_y_ == 0, "An ordinary left drop regressed during chord repair");
        idle(); history({ baseline, editor.document_.serialize() });
        prepare(); pan(); pointer(WM_MBUTTONUP, { 350, 325 });
        require(editor.view_x_ == -50 && editor.view_y_ == -25 && editor.document_.serialize() == baseline,
            "An ordinary middle drop regressed during chord repair");
        idle(); history({ baseline });

        keyboard.restore(); editor.cancel_drag();
        editor.document_.load(original_path); editor.selection_ = original_selection; editor.refresh();
        editor.zoom_ = original_zoom; editor.view_x_ = original_x; editor.view_y_ = original_y; editor.snap_ = original_snap;
        SetFocus(original_focus);
        require(cases == 10 && editor.document_.serialize() == original_document &&
            editor.document_.path() == original_path && !editor.document_.dirty() &&
            !editor.document_.can_undo() && !editor.document_.can_redo() && !editor.pending() &&
            editor.selection_.has_value() == original_selection.has_value() &&
            (!original_selection || same_entry(*editor.selection_, *original_selection)) &&
            editor.zoom_ == original_zoom && editor.view_x_ == original_x && editor.view_y_ == original_y &&
            editor.snap_ == original_snap && !editor.dragging_ && !editor.resizing_ && !editor.panning_ &&
            GetFocus() == original_focus && GetCapture() == nullptr && !IsWindowVisible(editor.window_),
            "Button chord qualification did not restore its clean hidden caller shell/view/focus");
        std::cout << "PASS native button chords: " << cases << " owned gesture cases, both button/release orders, "
            "move/resize, empty-space and pending-property handoffs, ordinary drops, exact history/source and caller restoration\n";
    }
}
