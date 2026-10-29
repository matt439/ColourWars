#pragma once

// Include after gesture_checks.h. The first WM_COMMAND uses a planned unused
// ID so this header builds against the old shell without a production stub.
namespace colourwars_editor
{
    inline void EditorDpiProbe::check_align_origin(Editor& editor, const std::filesystem::path& scratch)
    {
        constexpr int alignment_command = static_cast<int>(grid_size) + 1;
        const auto original_path = editor.document_.path();
        const std::string original_document = editor.document_.serialize();
        const auto original_selection = editor.selection_;
        const double original_zoom = editor.zoom_, original_x = editor.view_x_, original_y = editor.view_y_;
        const int original_snap = editor.snap_;
        const LRESULT original_grid = SendMessageW(editor.grid_, CB_GETCURSEL, 0, 0);
        const HWND original_focus = GetFocus();
        std::string original_property;
        if (editor.property_index_ >= 0 && static_cast<std::size_t>(editor.property_index_) < editor.properties_.size())
            original_property = editor.properties_[static_cast<std::size_t>(editor.property_index_)].path;
        require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
            !editor.document_.can_redo() && GetCapture() == nullptr && !editor.dragging_ &&
            !editor.resizing_ && !editor.panning_ && !IsWindowVisible(editor.window_),
            "Alignment qualification needs a clean private hidden shell");
        GestureKeyboardState keyboard;
        keyboard.shift(false);
        const auto bytes = [](const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Could not read a private native alignment stage");
            const std::string text{ std::istreambuf_iterator<char>(input), {} };
            require(!input.bad(), "Could not read a complete native alignment stage");
            return text;
        };
        const auto choose_grid = [&](int index, int expected)
        {
            require(SendMessageW(editor.grid_, CB_SETCURSEL, static_cast<WPARAM>(index), 0) != CB_ERR,
                "Could not select a native alignment grid");
            SendMessageW(editor.window_, WM_COMMAND, MAKEWPARAM(grid_size, CBN_SELCHANGE),
                reinterpret_cast<LPARAM>(editor.grid_));
            require(editor.snap_ == expected, "Native grid notification did not select its literal world spacing");
        };
        const auto pointer = [&](UINT message, POINT point, WPARAM buttons = 0)
        { SendMessageW(editor.canvas_, message, buttons, MAKELPARAM(point.x, point.y)); };
        const auto idle = [&]
        {
            require(GetCapture() == nullptr && !editor.dragging_ && !editor.resizing_ && !editor.panning_,
                "Alignment retained an old pointer gesture or capture");
        };
        std::filesystem::path stage;
        std::string baseline, disk;
        std::optional<Entry> selected;
        const auto prepare = [&](const std::filesystem::path& path, const Entry& entry)
        {
            editor.cancel_drag(); editor.document_.load(path); editor.selection_ = entry; editor.refresh();
            editor.zoom_ = 1; editor.view_x_ = 20; editor.view_y_ = 40;
            choose_grid(2, 25);
            stage = path; baseline = editor.document_.serialize(); disk = bytes(path); selected = entry;
            require(!editor.pending() && !editor.document_.dirty() && !editor.document_.can_undo() &&
                !editor.document_.can_redo(), "Native alignment fixture did not load a clean private baseline");
            idle();
        };
        const auto retained = [&]
        {
            require(editor.document_.path() == stage && bytes(stage) == disk &&
                editor.selection_.has_value() == selected.has_value() &&
                (!selected || same_entry(*editor.selection_, *selected)) && !editor.pending() &&
                editor.zoom_ == 1 && editor.view_x_ == 20 && editor.view_y_ == 40 && !IsWindowVisible(editor.window_),
                "Alignment changed private disk/path, selection, viewport or the hidden boundary");
            idle();
        };
        const auto menu_enabled = [&](bool expected)
        {
            const HMENU edit = GetSubMenu(editor.menu_, 1);
            const UINT state = GetMenuState(edit, alignment_command, MF_BYCOMMAND);
            require(state != 0xffffffffu && ((state & (MF_DISABLED | MF_GRAYED)) == 0) == expected,
                "Align origin menu availability does not follow the selected entry");
        };
        const auto command = [&]
        { SendMessageW(editor.window_, WM_COMMAND, static_cast<WPARAM>(alignment_command), 0); };
        const auto one_edit = [&]
        {
            const std::string aligned = editor.document_.serialize();
            require(editor.document_.dirty() && editor.document_.can_undo() && !editor.document_.can_redo(),
                "Native alignment did not retain its one admitted edit");
            retained(); menu_enabled(true);
            require(editor.document_.undo() && editor.document_.serialize() == baseline &&
                !editor.document_.dirty() && !editor.document_.can_undo() && editor.document_.can_redo() &&
                editor.document_.redo() && editor.document_.serialize() == aligned &&
                editor.document_.dirty() && editor.document_.can_undo() && !editor.document_.can_redo(),
                "Native alignment added an edit or lost exact undo/redo states");
            retained();
        };
        std::ofstream evidence(scratch / "alignment-native.csv", std::ios::trunc);
        require(evidence.good(), "Could not retain native alignment evidence");
        evidence << "case,x,y,width,height\n";
        const auto record = [&](const char* name, const Rect& rectangle)
        {
            evidence << name << ',' << rectangle.x << ',' << rectangle.y << ',' << rectangle.width << ','
                << rectangle.height << '\n';
            evidence.flush();
            require(evidence.good(), "Could not retain a complete alignment row");
        };
        LevelDocument seed;
        const Entry object = seed.add_object("StructurePaintable", { 113, 137, 160, 80 });
        const auto ordinary_stage = scratch / "align-native-rectangle.json";
        seed.save(ordinary_stage);
        prepare(ordinary_stage, object);
        command();
        record("ordinary", editor.document_.rectangle(object));
        // Primary old-shell baseline: the unused command is a no-op. Check the
        // actual literal geometry before any new menu/gesture expectations.
        require(same_gesture_rectangle(editor.document_.rectangle(object), { 125, 125, 160, 80 }),
            "Align origin command did not move the off-grid rectangle to (125,125).");
        one_edit();

        LevelDocument negative_seed;
        const Entry negative = negative_seed.add_object("Visual", { -12.5, -12.5, 80, 40 });
        const auto negative_stage = scratch / "align-native-negative.json";
        negative_seed.save(negative_stage);
        prepare(negative_stage, negative); keyboard.shift(true); command();
        require(same_gesture_rectangle(editor.document_.rectangle(negative), { -25, -25, 80, 40 }),
            "Explicit alignment followed Shift bypass or rounded negative ties toward zero");
        record("shift-negative", editor.document_.rectangle(negative)); one_edit(); keyboard.shift(false);

        LevelDocument slab_seed;
        const Entry slab = slab_seed.add_object("FixedSlabRight", { 513, 527, 1, 1 });
        const auto slab_stage = scratch / "align-native-slab.json";
        slab_seed.save(slab_stage);
        prepare(slab_stage, slab); command();
        bool slab_x = false, slab_y = false, slab_rise = false;
        for (const Property& property : editor.document_.properties(slab))
        {
            if (property.path == "slab/x") slab_x = property.numeric && std::stod(property.value) == 525;
            if (property.path == "slab/y") slab_y = property.numeric && std::stod(property.value) == 525;
            if (property.path == "slab/rise") slab_rise = property.value == "right";
        }
        require(slab_x && slab_y && slab_rise,
            "Native slab alignment used its reconstructed bounds instead of its authored origin");
        record("slab-authored-origin", editor.document_.rectangle(slab)); one_edit();

        // Command dispatch retains the existing preview barrier, admits the
        // pending X first, and then computes alignment from that new position.
        prepare(ordinary_stage, object);
        pointer(WM_LBUTTONDOWN, { 120, 120 }, MK_LBUTTON);
        pointer(WM_MOUSEMOVE, { 145, 145 }, MK_LBUTTON);
        require(editor.dragging_ && GetCapture() == editor.canvas_ &&
            !same_gesture_rectangle(editor.drag_preview_, { 113, 137, 160, 80 }),
            "Alignment interruption fixture lacks a distinct uncommitted preview");
        int x_row = -1;
        for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            if (editor.properties_[index].path == "rectangle/x") x_row = static_cast<int>(index);
        require(x_row >= 0, "Native alignment fixture lacks its X inspector row");
        editor.select_property(x_row); SetWindowTextW(editor.value_edit_, L"163");
        require(editor.pending(), "Native alignment fixture did not establish a pending X edit");
        command();
        require(same_gesture_rectangle(editor.document_.rectangle(object), { 175, 125, 160, 80 }),
            "Alignment did not use the admitted inspector position");
        const std::string aligned_pending = editor.document_.serialize();
        pointer(WM_LBUTTONUP, { 220, 200 });
        require(editor.document_.serialize() == aligned_pending && editor.document_.undo() &&
            same_gesture_rectangle(editor.document_.rectangle(object), { 163, 137, 160, 80 }),
            "A late release overwrote alignment or its first undo lost the pending property");
        const std::string pending_only = editor.document_.serialize();
        require(editor.document_.undo() && editor.document_.serialize() == baseline &&
            !editor.document_.dirty() && !editor.document_.can_undo() && editor.document_.redo() &&
            editor.document_.serialize() == pending_only && editor.document_.redo() &&
            editor.document_.serialize() == aligned_pending && !editor.document_.can_redo(),
            "Pending alignment did not retain exactly two history entries");
        retained(); record("pending-and-cancelled-preview", editor.document_.rectangle(object));

        prepare(ordinary_stage, object);
        editor.document_.set_rectangle(object, { 125, 125, 160, 80 });
        const std::string aligned_noop = editor.document_.serialize();
        editor.document_.set_rectangle(object, { 150, 125, 160, 80 });
        const std::string future = editor.document_.serialize();
        require(editor.document_.undo(), "Native no-op fixture needs a redo branch");
        editor.refresh(true); command();
        require(editor.document_.serialize() == aligned_noop && editor.document_.can_undo() &&
            editor.document_.can_redo() && editor.document_.undo() && editor.document_.serialize() == baseline &&
            !editor.document_.dirty() && !editor.document_.can_undo() && editor.document_.redo() &&
            editor.document_.serialize() == aligned_noop && editor.document_.redo() &&
            editor.document_.serialize() == future && !editor.document_.can_redo(),
            "Already-aligned native command rewrote data or truncated the prepared redo branch");
        retained(); record("aligned-noop-redo", editor.document_.rectangle(object));

        prepare(ordinary_stage, Entry{ EntryKind::level, 0, "Level settings" });
        menu_enabled(false); command();
        require(editor.document_.serialize() == baseline && !editor.document_.dirty() &&
            !editor.document_.can_undo() && !editor.document_.can_redo(),
            "Level metadata alignment changed a bounds rectangle or history");
        retained(); editor.selection_.reset(); selected.reset(); editor.command(alignment_command);
        require(!editor.selection_ && editor.document_.serialize() == baseline && !editor.document_.can_undo(),
            "Alignment without a selection manufactured geometry or selected another entry");
        retained();

        const auto refused_command = [&](const char* diagnostic)
        {
            bool refused = false;
            try { editor.command(alignment_command); }
            catch (const std::exception& error)
            { refused = std::string(error.what()).find(diagnostic) != std::string::npos; }
            require(refused && editor.document_.serialize() == baseline && !editor.document_.dirty() &&
                !editor.document_.can_undo() && !editor.document_.can_redo(),
                "Direct native alignment did not refuse its known geometry atomically");
            retained(); menu_enabled(true);
        };
        LevelDocument spawn_seed;
        const Entry spawn{ EntryKind::spawn_a, 0, "Team A spawn 1" };
        spawn_seed.set_rectangle(spawn, { 1490, 1075, 52, 120 });
        const auto spawn_stage = scratch / "align-native-refused-spawn.json";
        spawn_seed.save(spawn_stage);
        prepare(spawn_stage, spawn);
        pointer(WM_MBUTTONDOWN, { 100, 100 }, MK_MBUTTON);
        require(editor.panning_ && GetCapture() == editor.canvas_, "Refusal fixture did not establish its older pan");
        refused_command("Team A spawns on the left half");
        pointer(WM_MBUTTONUP, { 120, 120 }); retained();

        LevelDocument mover_seed(std::filesystem::path(COLOURWARS_CONTENT_DIR) / L"tuning.json");
        const auto initial_entries = mover_seed.entries();
        for (auto entry = initial_entries.rbegin(); entry != initial_entries.rend(); ++entry)
            if (entry->kind == EntryKind::collision || entry->kind == EntryKind::decoration) mover_seed.erase(*entry);
        mover_seed.add_object("Structure", { 548, 500, 8, 20 });
        const Entry mover = mover_seed.add_object("MovingPaintable", { 612, 600, 320, 20 });
        const auto mover_stage = scratch / "align-native-refused-mover.json";
        mover_seed.save(mover_stage);
        prepare(mover_stage, mover); refused_command("standing envelope intersects");

        keyboard.restore(); editor.cancel_drag();
        editor.document_.load(original_path); editor.selection_ = original_selection; editor.refresh();
        if (!original_selection) editor.selection_.reset();
        for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            if (editor.properties_[index].path == original_property) editor.select_property(static_cast<int>(index));
        SendMessageW(editor.grid_, CB_SETCURSEL, static_cast<WPARAM>(original_grid), 0);
        editor.snap_ = original_snap; editor.zoom_ = original_zoom; editor.view_x_ = original_x; editor.view_y_ = original_y;
        SetFocus(original_focus);
        require(editor.document_.serialize() == original_document && editor.document_.path() == original_path &&
            editor.selection_.has_value() == original_selection.has_value() &&
            (!original_selection || same_entry(*editor.selection_, *original_selection)) &&
            !editor.document_.dirty() && !editor.document_.can_undo() && !editor.document_.can_redo() &&
            !editor.pending() && GetFocus() == original_focus &&
            SendMessageW(editor.grid_, CB_GETCURSEL, 0, 0) == original_grid && !IsWindowVisible(editor.window_),
            "Alignment qualification did not restore the complete private caller shell");
        idle();
        std::cout << "PASS native align origin: 8 contexts, literal rectangle/negative Shift/slab origin, pending edit "
            "and preview cancellation, no-op redo, menu eligibility, spawn/mover refusal, private source and caller restoration\n";
    }
}
