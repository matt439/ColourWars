#pragma once

namespace colourwars_editor
{
    inline void EditorDpiProbe::check_group_move(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto original_path = editor.document_.path();
        const auto original_selection = editor.selection_;
        const double original_zoom = editor.zoom_, original_x = editor.view_x_, original_y = editor.view_y_;
        const int original_snap = editor.snap_;
        const HWND original_focus = GetFocus();
        GestureKeyboardState keyboard;
        keyboard.shift(false);

        const auto stage = scratch / "group-move.json";
        LevelDocument seed;
        const auto entries = seed.entries();
        for (auto it = entries.rbegin(); it != entries.rend(); ++it)
            if (it->kind == EntryKind::collision || it->kind == EntryKind::decoration) seed.erase(*it);
        const Rect left{ 100, 100, 80, 60 }, right{ 260, 110, 120, 80 }, third{ 480, 130, 80, 80 };
        const Entry a = seed.add_object("Structure", left);
        const Entry b = seed.add_object("Visual", right);
        const Entry c = seed.add_object("Structure", third);
        seed.save(stage);
        editor.document_.load(stage);
        editor.selection_ = a;
        editor.refresh();
        editor.zoom_ = 1; editor.view_x_ = editor.view_y_ = 0; editor.snap_ = 25;
        const std::string baseline = editor.document_.serialize();
        const auto pointer = [&](UINT message, POINT point, WPARAM buttons = 0)
        { SendMessageW(editor.canvas_, message, buttons, MAKELPARAM(point.x, point.y)); };
        const auto click = [&](POINT point, WPARAM modifier = 0)
        {
            pointer(WM_LBUTTONDOWN, point, MK_LBUTTON | modifier);
            pointer(WM_LBUTTONUP, point, modifier);
        };
        const auto group = [&]
        {
            click({ 120, 120 });
            click({ 280, 130 }, MK_CONTROL | MK_SHIFT);
            click({ 500, 150 }, MK_CONTROL | MK_SHIFT);
            click({ 500, 150 }, MK_CONTROL | MK_SHIFT);
            require(!editor.dragging_ && GetCapture() != editor.canvas_ &&
                editor.document_.serialize() == baseline && !editor.document_.can_undo(),
                "Adding and removing group members edited the saved stage or retained capture");
        };
        const auto check_offset = [&](double x, double y)
        {
            require(same_gesture_rectangle(editor.document_.rectangle(a), { left.x + x, left.y + y, left.width, left.height }) &&
                same_gesture_rectangle(editor.document_.rectangle(b), { right.x + x, right.y + y, right.width, right.height }) &&
                same_gesture_rectangle(editor.document_.rectangle(c), third),
                "Group movement did not translate exactly the two retained members by one common delta");
        };
        const auto drag = [&]
        {
            pointer(WM_LBUTTONDOWN, { 120, 120 }, MK_LBUTTON);
            pointer(WM_MOUSEMOVE, { 170, 145 }, MK_LBUTTON);
            require(editor.dragging_ && !editor.resizing_ && GetCapture() == editor.canvas_ &&
                editor.document_.serialize() == baseline && !editor.document_.can_undo(),
                "Dragging a group member did not start a captured, uncommitted preview");
        };
        const auto check_history = [&]
        {
            const std::string moved = editor.document_.serialize();
            require(editor.document_.dirty() && moved != baseline && !editor.document_.can_redo(),
                "Group movement did not create an edit");
            SendMessageW(editor.window_, WM_COMMAND, edit_undo, 0);
            require(editor.document_.serialize() == baseline && !editor.document_.dirty() &&
                !editor.document_.can_undo() && editor.document_.can_redo(),
                "One editor Undo did not restore the complete saved group");
            SendMessageW(editor.window_, WM_COMMAND, edit_redo, 0);
            require(editor.document_.serialize() == moved && !editor.document_.can_redo(),
                "One editor Redo did not restore the complete moved group");
        };
        group();
        drag();
        SendMessageW(editor.canvas_, WM_KEYDOWN, VK_ESCAPE, 0);
        pointer(WM_LBUTTONUP, { 170, 145 });
        require(!editor.dragging_ && GetCapture() != editor.canvas_ && editor.document_.serialize() == baseline &&
            !editor.document_.dirty() && !editor.document_.can_undo(), "Escape and late release committed a group preview");
        drag();
        pointer(WM_LBUTTONUP, { 170, 145 });
        check_offset(50, 25);
        check_history();

        // Clicking the unselected third entry returns to ordinary single selection.
        click({ 500, 150 });
        editor.document_.load(stage); editor.refresh();
        group();
        keyboard.shift(true);
        SendMessageW(editor.canvas_, WM_KEYDOWN, VK_UP, 0);
        keyboard.shift(false);
        check_offset(0, -1);
        check_history();
        click({ 120, 120 });
        click({ 280, 130 }, MK_CONTROL | MK_SHIFT);
        click({ 500, 150 });
        SendMessageW(editor.canvas_, WM_KEYDOWN, VK_RIGHT, 0);
        require(same_gesture_rectangle(editor.document_.rectangle(a), { 100, 99, 80, 60 }) &&
            same_gesture_rectangle(editor.document_.rectangle(b), { 260, 109, 120, 80 }) &&
            same_gesture_rectangle(editor.document_.rectangle(c), { 505, 130, 80, 80 }),
            "Plain selection after a group retained stale members on a grid nudge");

        const auto sluice = scratch / "group-contents-sluice.json";
        std::filesystem::copy_file(std::filesystem::path(COLOURWARS_CONTENT_DIR) / "levels" / "sluice_works.json",
            sluice, std::filesystem::copy_options::overwrite_existing);
        editor.document_.load(sluice);
        editor.select(editor.document_.entries().front());
        const std::string sluice_saved = editor.document_.serialize();
        const auto key = [&](WPARAM virtual_key, bool control = false)
        {
            keyboard.modifiers(false, control);
            MSG message{};
            message.hwnd = GetFocus(); message.message = WM_KEYDOWN; message.wParam = virtual_key; message.lParam = 1;
            editor.dispatch(message);
            keyboard.modifiers(false, false);
        };
        const auto select_row = [&](EntryKind kind, const std::string& label)
        {
            const auto found = std::find_if(editor.entries_.begin(), editor.entries_.end(),
                [&](const Entry& entry) { return entry.kind == kind && entry.label == label; });
            require(found != editor.entries_.end(), "Sluice Works lacks a named gauge assembly row");
            const Entry entry = *found;
            const auto target = found - editor.entries_.begin();
            const auto start = SendMessageW(editor.object_list_, LB_GETCARETINDEX, 0, 0);
            require(GetFocus() == editor.object_list_ && start >= 0, "Level Contents lost keyboard focus or its cursor");
            for (auto cursor = start; cursor != target; cursor += cursor < target ? 1 : -1)
                key(cursor < target ? VK_DOWN : VK_UP, true);
            require(SendMessageW(editor.object_list_, LB_GETCARETINDEX, 0, 0) == target,
                "Ctrl+arrow did not reach the requested Level Contents row");
            key(VK_SPACE);
            return entry;
        };
        SetFocus(editor.object_list_);
        const Entry station = select_row(EntryKind::station, "Station gauge_step_a");
        const auto coordinate = std::find_if(editor.properties_.begin(), editor.properties_.end(),
            [](const Property& property) { return property.path == "x"; });
        require(coordinate != editor.properties_.end(), "Selected gauge station lacks an X coordinate");
        editor.select_property(static_cast<int>(coordinate - editor.properties_.begin()));
        SetFocus(editor.value_edit_);
        SetWindowTextW(editor.value_edit_, L"invalid");
        require(editor.pending(), "Invalid station coordinate did not enter the pending inspector");
        key(VK_ESCAPE);
        require(!editor.pending() && editor.document_.serialize() == sluice_saved && !editor.document_.can_undo(),
            "Escape did not discard an invalid station coordinate before keyboard grouping");
        SetFocus(editor.object_list_);
        const Entry trestle = select_row(EntryKind::decoration, "gauge_trestle_a");
        const Entry step = select_row(EntryKind::collision, "gauge_step_a");
        require(editor.document_.serialize() == sluice_saved && !editor.document_.can_undo(),
            "Traversing Level Contents or grouping the gauge assembly edited the stage");
        key(VK_RETURN);
        require(GetFocus() == editor.canvas_, "Enter from Level Contents did not focus the canvas");
        const auto members = std::array{ station, trestle, step };
        const auto before = std::array{ editor.document_.rectangle(station), editor.document_.rectangle(trestle),
            editor.document_.rectangle(step) };
        // Small real moves avoid changing clearance to another authored mechanism.
        editor.snap_ = 1;
        for (int distance = 1; distance <= 2; ++distance)
        {
            key(VK_RIGHT);
            require(editor.group_.size() == members.size(), "A successful keyboard nudge dropped the group");
            for (size_t index = 0; index < members.size(); ++index)
            {
                auto expected = before[index];
                expected.x += distance;
                require(editor.is_selected(members[index]) &&
                    same_gesture_rectangle(editor.document_.rectangle(members[index]), expected),
                    "Keyboard nudging lost a selected gauge assembly member or moved it incorrectly");
            }
        }
        SendMessageW(editor.window_, WM_COMMAND, edit_undo, 0);
        SendMessageW(editor.window_, WM_COMMAND, edit_undo, 0);
        require(editor.document_.serialize() == sluice_saved && !editor.document_.can_undo() && !editor.document_.dirty(),
            "Two group nudges did not leave exactly two undo steps");

        // Ctrl+Down moves the dotted list caret without changing the blue
        // selection. Single-object commands must follow that visible selection.
        for (bool duplicate : { false, true })
        {
            editor.document_.load(stage); editor.select(a);
            SetFocus(editor.object_list_);
            const auto selected = SendMessageW(editor.object_list_, LB_GETCARETINDEX, 0, 0);
            key(VK_DOWN, true);
            const auto caret = SendMessageW(editor.object_list_, LB_GETCARETINDEX, 0, 0);
            require(caret == selected + 1 && SendMessageW(editor.object_list_, LB_GETSELCOUNT, 0, 0) == 1 &&
                SendMessageW(editor.object_list_, LB_GETSEL, selected, 0) == 1 &&
                SendMessageW(editor.object_list_, LB_GETSEL, caret, 0) == 0,
                "Keyboard browsing moved the highlighted selection instead of only its dotted caret");
            LevelDocument expected;
            expected.load(stage);
            if (duplicate) expected.duplicate(a);
            else expected.erase(a);
            key(duplicate ? 'D' : VK_DELETE, duplicate);
            require(editor.document_.serialize() == expected.serialize(),
                "Delete or Ctrl+D did not apply to the visibly selected Level Contents row");
        }

        editor.document_.load(stage); editor.select(a);
        const auto selected_row = SendMessageW(editor.object_list_, LB_GETCARETINDEX, 0, 0);
        const auto other_row = selected_row + 1;
        const Entry other = editor.entries_.at(static_cast<size_t>(other_row));
        SendMessageW(editor.object_list_, LB_SETTOPINDEX, selected_row, 0);
        RECT row_rect{};
        require(SendMessageW(editor.object_list_, LB_GETITEMRECT, other_row, reinterpret_cast<LPARAM>(&row_rect)) != LB_ERR,
            "Could not locate the adjacent Level Contents row for a Ctrl+click");
        const LPARAM row_point = MAKELPARAM(row_rect.left + 5, (row_rect.top + row_rect.bottom) / 2);
        const auto ctrl_click_row = [&]
        {
            keyboard.modifiers(false, true);
            SendMessageW(editor.object_list_, WM_LBUTTONDOWN, MK_LBUTTON | MK_CONTROL, row_point);
            SendMessageW(editor.object_list_, WM_LBUTTONUP, MK_CONTROL, row_point);
            keyboard.modifiers(false, false);
        };
        ctrl_click_row();
        require(editor.group_.size() == 2 && editor.is_selected(a) && editor.is_selected(other) &&
            SendMessageW(editor.object_list_, LB_GETSELCOUNT, 0, 0) == 2,
            "Ctrl+click in Level Contents did not select the two highlighted entries");
        ctrl_click_row();
        require(editor.group_.empty() && editor.is_selected(a) && !editor.is_selected(other) &&
            SendMessageW(editor.object_list_, LB_GETSELCOUNT, 0, 0) == 1 &&
            editor.document_.serialize() == baseline && !editor.document_.can_undo(),
            "Second Ctrl+click in Level Contents did not remove exactly one member without editing");

        keyboard.restore();
        editor.document_.load(original_path); editor.selection_ = original_selection; editor.refresh();
        editor.zoom_ = original_zoom; editor.view_x_ = original_x; editor.view_y_ = original_y;
        editor.snap_ = original_snap; SetFocus(original_focus);
        std::cout << "PASS native group selection, move and fine nudge, cancel, undo/redo and single selection\n";
    }
}
