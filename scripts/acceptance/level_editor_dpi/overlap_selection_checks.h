#pragma once

// Include after gesture_checks.h. These real hidden-canvas messages use only
// MK_CONTROL; they neither synthesize physical keys nor change global input.
namespace colourwars_editor
{
    inline void EditorDpiProbe::check_overlap_selection(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto original_path = editor.document_.path();
        const std::string original_document = editor.document_.serialize();
        const auto original_selection = editor.selection_;
        const double original_zoom = editor.zoom_, original_x = editor.view_x_, original_y = editor.view_y_;
        const int original_snap = editor.snap_;
        const HWND original_focus = GetFocus();
        std::string original_property;
        if (editor.property_index_ >= 0 && static_cast<std::size_t>(editor.property_index_) < editor.properties_.size())
            original_property = editor.properties_[static_cast<std::size_t>(editor.property_index_)].path;
        require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
            !editor.document_.can_redo() && !editor.dragging_ && !editor.resizing_ && !editor.panning_ &&
            GetCapture() == nullptr && !IsWindowVisible(editor.window_),
            "Overlap selection needs a clean private hidden caller without capture");
        GestureKeyboardState keyboard;
        keyboard.shift(false);
        std::array<BYTE, 256> local_keys{};
        require(GetKeyboardState(local_keys.data()) != 0, "Could not retain overlap selection thread keys");
        const auto pointer = [&](UINT message, POINT point, WPARAM buttons = 0)
        { SendMessageW(editor.canvas_, message, buttons, MAKELPARAM(point.x, point.y)); };
        const auto bytes = [](const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Could not read an overlap selection private stage");
            const std::string text{ std::istreambuf_iterator<char>(input), {} };
            require(!input.bad(), "Could not read a complete overlap selection private stage");
            return text;
        };
        const auto clear_geometry = [](LevelDocument& seed)
        {
            const auto entries = seed.entries();
            for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry)
                if (entry->kind == EntryKind::collision || entry->kind == EntryKind::decoration) seed.erase(*entry);
        };
        const auto idle = [&]
        {
            require(!editor.dragging_ && !editor.resizing_ && !editor.panning_ && GetCapture() == nullptr,
                "Ctrl-click retained a pointer gesture or capture");
        };
        std::filesystem::path current_stage;
        std::string baseline, disk;
        const auto prepare = [&](const std::filesystem::path& stage, const std::optional<Entry>& selected)
        {
            editor.cancel_drag(); editor.document_.load(stage);
            editor.selection_ = selected; editor.refresh();
            if (!selected) editor.selection_.reset();
            editor.zoom_ = 1; editor.view_x_ = editor.view_y_ = 0; editor.snap_ = 25;
            current_stage = stage; baseline = editor.document_.serialize(); disk = bytes(stage);
            require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
                !editor.document_.can_redo(), "Overlap selection private fixture is not clean");
            idle();
        };
        const auto preserved = [&](const std::string& expected, bool edited)
        {
            require(editor.document_.serialize() == expected && editor.document_.path() == current_stage &&
                editor.document_.dirty() == edited && editor.document_.can_undo() == edited &&
                !editor.document_.can_redo() && bytes(current_stage) == disk && !IsWindowVisible(editor.window_),
                "Overlap selection changed geometry/history/private disk bytes or its hidden boundary");
        };
        const auto selected_is = [&](const std::optional<Entry>& expected)
        {
            return editor.selection_.has_value() == expected.has_value() &&
                (!expected || same_entry(*editor.selection_, *expected));
        };
        std::size_t queries = 0;
        std::ofstream evidence(scratch / "overlap-selection.csv", std::ios::trunc);
        require(evidence.good(), "Could not retain overlap selection evidence");
        evidence << "case,x,y,expected_kind,expected_index,actual_kind,actual_index\n";
        const auto ctrl = [&](const char* name, POINT point, const std::optional<Entry>& expected,
            const char* diagnostic, bool pending_edit = false)
        {
            const double x = editor.view_x_, y = editor.view_y_, zoom = editor.zoom_;
            pointer(WM_LBUTTONDOWN, point, MK_CONTROL | MK_LBUTTON);
            ++queries;
            evidence << name << ',' << point.x << ',' << point.y << ','
                << (expected ? static_cast<int>(expected->kind) : -1) << ',' << (expected ? expected->index : 0) << ','
                << (editor.selection_ ? static_cast<int>(editor.selection_->kind) : -1) << ','
                << (editor.selection_ ? editor.selection_->index : 0) << '\n';
            evidence.flush();
            // The first old-source oracle is the actual buried entry, before
            // checking the selection-only gesture/capture consequences.
            require(selected_is(expected), diagnostic);
            idle();
            const std::string admitted = editor.document_.serialize();
            pointer(WM_MOUSEMOVE, { point.x + 37, point.y + 23 }, MK_CONTROL | MK_LBUTTON);
            pointer(WM_LBUTTONUP, { point.x + 37, point.y + 23 }, MK_CONTROL);
            pointer(WM_MOUSEMOVE, point);
            require(selected_is(expected) && editor.view_x_ == x && editor.view_y_ == y && editor.zoom_ == zoom &&
                !editor.pending(), "Ctrl-click movement/release changed selection, view or pending state");
            idle(); preserved(pending_edit ? admitted : baseline, pending_edit);
        };
        const Entry level{ EntryKind::level, 0, "Level settings" };
        const Entry bounds{ EntryKind::bounds, 1, "Camera bounds" };
        const Rect body{ 100, 100, 160, 120 };
        LevelDocument stack;
        clear_geometry(stack);
        const Entry below = stack.add_object("Visual", body);
        const Entry older = stack.add_object("StructurePaintable", body);
        const Entry newer = stack.add_object("Structure", body);
        const Entry single = stack.add_object("Structure", { 400, 100, 100, 100 });
        stack.set_rectangle(bounds, body);
        const auto stacked_stage = scratch / "overlap-stack.json";
        stack.save(stacked_stage);
        prepare(stacked_stage, newer);
        ctrl("buried-collider", { 150, 150 }, older,
            "Ctrl-click did not select the next buried canvas object.");
        ctrl("buried-decoration", { 150, 150 }, below, "Ctrl-click could not reach an obscured decoration");
        ctrl("wrap-to-top", { 150, 150 }, newer, "Ctrl-click did not wrap to the first eligible object");
        ctrl("repeat-next", { 150, 150 }, older, "Repeated Ctrl-click changed the established overlap order");
        prepare(stacked_stage, level);
        ctrl("noneligible-level", { 150, 150 }, newer, "A noneligible selection did not start at the first hit");
        prepare(stacked_stage, bounds);
        ctrl("bounds-handle-bypass", { 260, 220 }, newer, "Ctrl-click resized or retained selected bounds instead of cycling bodies");
        prepare(stacked_stage, single);
        ctrl("single-hit", { 450, 150 }, single, "Ctrl-click lost its only eligible object");
        ctrl("no-hit-keeps-selection", { 750, 400 }, single, "Ctrl-click on empty space changed the current selection");
        prepare(stacked_stage, std::nullopt);
        ctrl("no-hit-empty-selection", { 750, 400 }, std::nullopt, "Empty-space Ctrl-click manufactured a selection");

        // The ordinary path keeps both its reverse-authored priority and the
        // selected bounds' independent resize handle behavior.
        prepare(stacked_stage, older);
        pointer(WM_LBUTTONDOWN, { 150, 150 }, MK_LBUTTON);
        require(selected_is(newer) && editor.dragging_ && !editor.resizing_ && GetCapture() == editor.canvas_,
            "Ordinary click changed its first-hit priority or move gesture");
        pointer(WM_LBUTTONUP, { 150, 150 }); idle(); preserved(baseline, false);
        prepare(stacked_stage, bounds);
        pointer(WM_LBUTTONDOWN, { 260, 220 }, MK_LBUTTON);
        require(selected_is(bounds) && editor.dragging_ && editor.resizing_ && GetCapture() == editor.canvas_,
            "Ordinary selected-bounds handle lost its existing resize behavior");
        pointer(WM_LBUTTONUP, { 260, 220 }); idle(); preserved(baseline, false);

        // A modifier selection supersedes an existing preview/pan, and later
        // releases cannot revive it or turn the selection into a document edit.
        prepare(stacked_stage, newer);
        pointer(WM_LBUTTONDOWN, { 150, 150 }, MK_LBUTTON);
        pointer(WM_MOUSEMOVE, { 175, 175 }, MK_LBUTTON);
        require(editor.dragging_ && !same_gesture_rectangle(editor.drag_preview_, body),
            "Ctrl-click interruption fixture lacks a distinct uncommitted preview");
        ctrl("cancel-preview", { 150, 150 }, older, "Ctrl-click did not select after discarding an older preview");
        prepare(stacked_stage, newer);
        pointer(WM_MBUTTONDOWN, { 150, 150 }, MK_MBUTTON);
        require(editor.panning_ && GetCapture() == editor.canvas_, "Ctrl-click pan fixture did not take capture");
        ctrl("cancel-pan", { 150, 150 }, older, "Ctrl-click did not select after ending an older pan");
        pointer(WM_MBUTTONUP, { 200, 180 }); idle(); preserved(baseline, false);

        LevelDocument categories;
        clear_geometry(categories);
        const Rect common{ 1480, 400, 120, 120 };
        const Entry category_decor = categories.add_object("Visual", common);
        const Entry category_collision = categories.add_object("Structure", common);
        const Entry a_first = categories.add_object("SpawnA", { 1495, 400, 1, 1 });
        const Entry a_last = categories.add_object("SpawnA", { 1498, 400, 1, 1 });
        const Entry b = categories.add_object("SpawnB", { 1500, 400, 1, 1 });
        const auto category_stage = scratch / "overlap-categories.json";
        categories.save(category_stage);
        prepare(category_stage, level); editor.view_x_ = 1300; editor.view_y_ = 200;
        const Entry category_order[] = { b, a_last, a_first, category_collision, category_decor, b };
        for (const Entry& expected : category_order)
            ctrl("category-order", { 210, 230 }, expected, "Ctrl-click changed spawn/collision/decoration or reverse-authored priority");

        LevelDocument ramp_seed;
        clear_geometry(ramp_seed);
        const Entry ramp_below = ramp_seed.add_object("Visual", { 100, 100, 160, 160 });
        const Entry ramp = ramp_seed.add_object("StructureRampRight", { 100, 100, 160, 160 });
        const auto ramp_stage = scratch / "overlap-ramp.json";
        ramp_seed.save(ramp_stage);
        prepare(ramp_stage, ramp);
        ctrl("ramp-empty-half", { 140, 140 }, ramp_below, "Ctrl-click included the empty half of a ramp");
        ctrl("ramp-edge", { 180, 180 }, ramp, "Ctrl-click lost the existing triangular ramp edge");
        ctrl("ramp-filled-half", { 220, 220 }, ramp_below, "Ctrl-click did not cycle from a filled ramp to its decoration");

        LevelDocument slab_seed;
        clear_geometry(slab_seed);
        const Entry slab_below = slab_seed.add_object("Visual", { 490, 410, 200, 120 });
        const Entry slab = slab_seed.add_object("FixedSlabRight", { 500, 500, 1, 1 });
        const auto slab_stage = scratch / "overlap-slab.json";
        slab_seed.save(slab_stage);
        prepare(slab_stage, slab);
        ctrl("slab-empty-corner", { 505, 425 }, slab_below, "Ctrl-click widened the slab to its bounding rectangle");
        // One pixel to either side of the authored (580,460) top avoids
        // depending on sub-pixel float reconstruction at the exact boundary.
        ctrl("slab-inside-edge", { 580, 461 }, slab, "Ctrl-click lost the slab interior beside its top edge");
        ctrl("slab-outside-edge", { 580, 459 }, slab_below, "Ctrl-click added rectangle tolerance outside the slab polygon");

        LevelDocument mover_seed(std::filesystem::path(COLOURWARS_CONTENT_DIR) / L"tuning.json");
        clear_geometry(mover_seed);
        const Entry mover_below = mover_seed.add_object("Visual", { 550, 550, 500, 100 });
        const Entry mover = mover_seed.add_object("MovingPaintable", { 600, 600, 320, 20 });
        mover_seed.set_property(mover, "motion/phase", "0.25");
        const auto mover_stage = scratch / "overlap-mover.json";
        mover_seed.save(mover_stage);
        prepare(mover_stage, mover); editor.view_x_ = editor.view_y_ = 400;
        ctrl("mover-overlap-dedup", { 320, 210 }, mover_below, "Ctrl-click counted a mover's anchor and initial pose twice");
        ctrl("mover-overlap-wrap", { 320, 210 }, mover, "Ctrl-click lost a mover after wrapping its deduplicated hits");
        ctrl("mover-anchor-only", { 230, 210 }, mover_below, "Ctrl-click lost the mover's supported anchor hit");
        ctrl("mover-initial-only", { 550, 210 }, mover, "Ctrl-click lost the mover's supported initial-pose hit");

        prepare(stacked_stage, newer);
        int height_row = -1;
        for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            if (editor.properties_[index].path == "rectangle/height") height_row = static_cast<int>(index);
        require(height_row >= 0, "Overlap fixture lacks its height inspector row");
        editor.select_property(height_row); SetWindowTextW(editor.value_edit_, L"145");
        require(editor.pending(), "Overlap fixture did not establish a pending inspector edit");
        ctrl("pending-before-cycle", { 150, 150 }, older, "Ctrl-click lost selection after pending-property admission", true);
        const std::string admitted = editor.document_.serialize();
        require(same_gesture_rectangle(editor.document_.rectangle(newer), { 100, 100, 160, 145 }) &&
            editor.document_.undo() && editor.document_.serialize() == baseline && !editor.document_.dirty() &&
            !editor.document_.can_undo() && editor.document_.can_redo() && editor.document_.redo() &&
            editor.document_.serialize() == admitted && editor.document_.dirty() && !editor.document_.can_redo(),
            "Ctrl-click changed pending-property geometry or its exact one-edit undo/redo branch");
        preserved(admitted, true);

        std::array<BYTE, 256> after_keys{};
        require(GetKeyboardState(after_keys.data()) != 0 && after_keys == local_keys,
            "MK_CONTROL canvas messages changed the probe thread's keyboard state");
        require(window_text(editor.hint_).find(L"Ctrl+click") != std::wstring::npos,
            "The native canvas hint does not expose overlap selection");
        keyboard.restore();
        editor.cancel_drag(); editor.document_.load(original_path); editor.selection_ = original_selection; editor.refresh();
        if (!original_selection) editor.selection_.reset();
        for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            if (editor.properties_[index].path == original_property) editor.select_property(static_cast<int>(index));
        editor.zoom_ = original_zoom; editor.view_x_ = original_x; editor.view_y_ = original_y; editor.snap_ = original_snap;
        SetFocus(original_focus);
        require(editor.document_.serialize() == original_document && editor.document_.path() == original_path &&
            editor.selection_.has_value() == original_selection.has_value() &&
            (!original_selection || same_entry(*editor.selection_, *original_selection)) &&
            !editor.document_.dirty() && !editor.document_.can_undo() && !editor.document_.can_redo() &&
            !editor.pending() && GetFocus() == original_focus && !IsWindowVisible(editor.window_),
            "Overlap qualification did not restore the complete private caller shell");
        idle();
        require(evidence.good(), "Could not retain complete overlap selection evidence");
        std::cout << "PASS native overlap selection: " << queries
            << " Ctrl-clicks, literal buried entries/wrap/categories, ramp/slab boundaries, mover deduplication, "
            "ordinary/bounds controls, pending edit/history, no pointer capture, private source and caller restoration\n";
    }
}
