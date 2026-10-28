#pragma once

// Included after the existing bitmap and gesture helpers. Expected body points
// and edge offsets are literal examples, not a second triangle classifier.
namespace colourwars_editor
{
    inline void EditorDpiProbe::check_ramp_hits(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto original_path = editor.document_.path();
        const auto original_selection = editor.selection_;
        const UINT original_dpi = editor.dpi_;
        RECT original_window{}; GetWindowRect(editor.window_, &original_window);
        const double original_zoom = editor.zoom_, original_x = editor.view_x_, original_y = editor.view_y_;
        const int original_snap = editor.snap_;
        const HWND original_focus = GetFocus();
        GestureKeyboardState keyboard;
        keyboard.shift(false);
        const Rect body{ 100, 100, 160, 160 };
        const RECT work = editor_monitor_work_area(MonitorFromWindow(editor.window_, MONITOR_DEFAULTTONEAREST));
        struct DpiCase { UINT dpi; int tolerance; int diagonal_near; int diagonal_far; };
        const DpiCase dpi_cases[] = { { 96, 3, 2, 3 }, { 144, 5, 3, 4 }, { 192, 6, 4, 5 } };
        struct ViewCase { double zoom, x, y; };
        const ViewCase views[] = { { 0.5, 0.375, 0.625 }, { 1.0, 3.25, 7.75 }, { 1.5, -0.25, 0.5 } };
        std::size_t queries = 0, contexts = 0, raster_samples = 0;
        std::ofstream evidence(scratch / "ramp-hits.csv", std::ios::trunc);
        require(evidence.good(), "Could not retain ramp hit-test evidence");
        evidence << "case,dpi,zoom,origin_x,origin_y,x,y,expected_kind,expected_index,actual_kind,actual_index\n";
        const auto change_dpi = [&](UINT dpi)
        {
            const HFONT previous_font = editor.font_;
            const UINT previous_dpi = editor.dpi_;
            const RECT suggested = initial_editor_rectangle(dpi, work);
            SendMessageW(editor.window_, WM_DPICHANGED, MAKEWPARAM(dpi, dpi), reinterpret_cast<LPARAM>(&suggested));
            if (previous_dpi != dpi)
            { require(GetObjectType(previous_font) == 0, "Input-activated DPI transition retained its replaced owned font"); }
            require(editor.dpi_ == dpi && !IsWindowVisible(editor.window_),
                "Ramp selection fixture failed its hidden native DPI transition");
        };
        const auto query = [&](const char* name, POINT point, const std::optional<Entry>& expected,
            const char* diagnostic)
        {
            const std::string before = editor.document_.serialize();
            const auto path = editor.document_.path();
            const bool dirty = editor.document_.dirty(), undo = editor.document_.can_undo(), redo = editor.document_.can_redo();
            const auto selection = editor.selection_;
            const double zoom = editor.zoom_, x = editor.view_x_, y = editor.view_y_;
            const UINT dpi = editor.dpi_;
            const auto actual = editor.hit(point);
            ++queries;
            evidence << name << ',' << dpi << ',' << zoom << ',' << x << ',' << y << ',' << point.x << ',' << point.y << ','
                << (expected ? static_cast<int>(expected->kind) : -1) << ',' << (expected ? expected->index : 0) << ','
                << (actual ? static_cast<int>(actual->kind) : -1) << ',' << (actual ? actual->index : 0) << '\n';
            evidence.flush();
            require(actual.has_value() == expected.has_value() && (!expected || same_entry(*actual, *expected)), diagnostic);
            require(editor.document_.serialize() == before && editor.document_.path() == path &&
                editor.document_.dirty() == dirty && editor.document_.can_undo() == undo && editor.document_.can_redo() == redo &&
                editor.selection_.has_value() == selection.has_value() && (!selection || same_entry(*editor.selection_, *selection)) &&
                editor.zoom_ == zoom && editor.view_x_ == x && editor.view_y_ == y && editor.dpi_ == dpi,
                "A ramp hit query changed document, selection, view or history");
        };
        const auto bytes = [](const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Could not inspect a private ramp stage");
            return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        };

        for (bool left : { false, true })
        {
            LevelDocument seed;
            const Entry below = seed.add_object("Structure", { 60, 60, 240, 240 });
            const Entry ramp = seed.add_object(left ? "StructureRampLeft" : "StructureRampRight", body);
            const auto stage = scratch / (left ? "ramp-left.json" : "ramp-right.json");
            seed.save(stage);
            const std::string disk = bytes(stage);
            for (const DpiCase& dpi_case : dpi_cases)
            {
                change_dpi(dpi_case.dpi);
                for (const ViewCase& view : views)
                {
                    editor.document_.load(stage);
                    const std::string saved = editor.document_.serialize();
                    editor.document_.set_property(ramp, "name", "First ramp label");
                    const std::string first = editor.document_.serialize();
                    editor.document_.set_property(ramp, "name", "Second ramp label");
                    const std::string second = editor.document_.serialize();
                    require(editor.document_.undo(), "Could not prepare the ramp query history branch");
                    editor.selection_ = ramp; editor.refresh();
                    editor.zoom_ = view.zoom; editor.view_x_ = view.x; editor.view_y_ = view.y;
                    const RECT screen = editor.screen_rect(body);
                    const LONG width = screen.right - screen.left, height = screen.bottom - screen.top;
                    require(width == height && width >= 80, "Ramp literal examples require a non-collapsed square");
                    const POINT inside{ screen.left + (left ? width / 4 : width * 3 / 4), screen.top + height * 3 / 4 };
                    const POINT empty{ screen.left + (left ? width * 3 / 4 : width / 4), screen.top + height / 4 };
                    // The first baseline assertion must expose the actual empty
                    // corner stealing the older rectangular object's selection.
                    query("empty-corner", empty, below, "Empty ramp corner intercepted an underlying rectangle");
                    query("body-interior", inside, ramp, "A drawn ramp interior did not select the ramp");
                    const POINT middle{ screen.left + width / 2, screen.top + height / 2 };
                    query("slope-edge", middle, ramp, "The drawn ramp slope edge was not selectable");
                    const LONG horizontal = left ? 1 : -1;
                    query("slope-tolerance", { middle.x + horizontal * dpi_case.diagonal_near,
                        middle.y - dpi_case.diagonal_near }, ramp, "Ramp slope lost its DPI-scaled pixel tolerance");
                    query("slope-outside", { middle.x + horizontal * dpi_case.diagonal_far,
                        middle.y - dpi_case.diagonal_far }, below, "Ramp slope tolerance extended beyond three logical pixels");
                    const LONG side = left ? screen.left : screen.right;
                    const LONG outward = left ? -1 : 1;
                    query("vertical-tolerance", { side + outward * dpi_case.tolerance, middle.y }, ramp,
                        "Ramp vertical edge did not include its exact distance tolerance");
                    query("vertical-outside", { side + outward * (dpi_case.tolerance + 1), middle.y }, below,
                        "Ramp vertical edge intercepted a point beyond its distance tolerance");
                    query("bottom-tolerance", { middle.x, screen.bottom + dpi_case.tolerance }, ramp,
                        "Ramp bottom edge did not include its exact distance tolerance");
                    query("bottom-outside", { middle.x, screen.bottom + dpi_case.tolerance + 1 }, below,
                        "Ramp bottom edge intercepted a point beyond its distance tolerance");
                    query("top-endpoint-tolerance", { side, screen.top - dpi_case.tolerance }, ramp,
                        "Ramp endpoint lost its bounded distance tolerance");
                    query("top-endpoint-outside", { side + outward * dpi_case.tolerance,
                        screen.top - dpi_case.tolerance }, below, "Ramp endpoint behaved like an unbounded edge line");

                    const auto found = std::find_if(editor.draw_items_.begin(), editor.draw_items_.end(),
                        [&](const DrawItem& item) { return same_entry(item.entry, ramp); });
                    require(found != editor.draw_items_.end(), "The actual ramp draw item disappeared");
                    MechanismBitmap bitmap(512, 512);
                    const auto selected = editor.selection_;
                    editor.selection_.reset();
                    editor.draw_item(bitmap.dc(), *found);
                    editor.selection_ = selected;
                    GdiFlush();
                    require(bitmap.pixel(inside.x, inside.y) == MechanismBitmap::colour(RGB(156, 110, 77)) &&
                        bitmap.pixel(empty.x, empty.y) == MechanismBitmap::colour(RGB(28, 36, 46)),
                        "Independent interior/empty hit examples disagree with actual GDI ramp body pixels");
                    raster_samples += 2; ++contexts;
                    if (view.zoom == 1.0)
                        bitmap.save(scratch / (std::string("ramp-body-") + (left ? "left-" : "right-") +
                            std::to_string(dpi_case.dpi) + ".bmp"));
                    require(editor.document_.serialize() == first && editor.document_.can_undo() && editor.document_.can_redo() &&
                        editor.document_.undo() && editor.document_.serialize() == saved && !editor.document_.dirty() &&
                        editor.document_.redo() && editor.document_.serialize() == first &&
                        editor.document_.redo() && editor.document_.serialize() == second &&
                        editor.document_.undo() && editor.document_.serialize() == first && bytes(stage) == disk,
                        "Ramp queries/drawing changed the complete prepared history or private source bytes");
                }
            }

            // Isolate these actual ramp/block draw items from unrelated spawn
            // minimum-size boxes. Full category priority is checked separately.
            // Legal geometry can collapse after zoom/clipping; its finite
            // endpoints must not turn into an infinite selectable edge line.
            const auto full_items = editor.draw_items_;
            editor.draw_items_.erase(std::remove_if(editor.draw_items_.begin(), editor.draw_items_.end(),
                [&](const DrawItem& item) { return !same_entry(item.entry, ramp) && !same_entry(item.entry, below); }),
                editor.draw_items_.end());
            editor.zoom_ = 0.005; editor.view_x_ = editor.view_y_ = 90;
            query("collapsed-point-near", { 0, 0 }, ramp, "A collapsed ramp point lost its finite hit tolerance");
            query("collapsed-point-far", { 100, 0 }, std::nullopt, "A collapsed ramp point intercepted distant geometry");
            editor.zoom_ = 0.005; editor.view_x_ = 90; editor.view_y_ = -90;
            query("collapsed-segment-near", { 0, 0 }, ramp, "A collapsed ramp segment lost its finite hit tolerance");
            query("collapsed-segment-far", { 0, 100 }, std::nullopt, "A collapsed ramp segment accepted an infinite edge extension");
            editor.zoom_ = 10; editor.view_x_ = -200000; editor.view_y_ = -200000;
            query("clipped-point-near", { 1000000, 1000000 }, ramp, "A clipped ramp point lost its finite hit tolerance");
            query("clipped-point-far", { 999000, 1000000 }, std::nullopt, "A clipped ramp point intercepted distant geometry");
            editor.draw_items_ = full_items;

            change_dpi(96);
            editor.document_.load(stage); editor.selection_ = ramp; editor.refresh();
            editor.zoom_ = 1; editor.view_x_ = editor.view_y_ = 0; editor.snap_ = 25;
            const std::string before_resize = editor.document_.serialize();
            const RECT screen = editor.screen_rect(body);
            const POINT handle{ screen.right + 7, screen.bottom + 7 };
            query("selected-handle-outside-body", handle, below, "Resize fixture must begin outside ramp body tolerance");
            SendMessageW(editor.canvas_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(handle.x, handle.y));
            require(editor.dragging_ && editor.resizing_ && editor.selection_ && same_entry(*editor.selection_, ramp) &&
                GetCapture() == editor.canvas_, "Selected ramp resize handle lost its independent input priority");
            SendMessageW(editor.canvas_, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(handle.x + 25, handle.y + 25));
            SendMessageW(editor.canvas_, WM_LBUTTONUP, 0, MAKELPARAM(handle.x + 25, handle.y + 25));
            require(same_gesture_rectangle(editor.document_.rectangle(ramp), { 100, 100, 185, 185 }) &&
                !editor.dragging_ && !editor.resizing_ && GetCapture() != editor.canvas_,
                "Selected ramp resize did not commit its geometry and release capture");
            const std::string resized = editor.document_.serialize();
            require(editor.document_.undo() && editor.document_.serialize() == before_resize && !editor.document_.dirty() &&
                !editor.document_.can_undo() && editor.document_.redo() && editor.document_.serialize() == resized &&
                !editor.document_.can_redo() && bytes(stage) == disk,
                "Ramp resize changed its exact single-edit history or saved source");
        }

        // Existing category and reverse-authored priority still decide overlaps.
        editor.document_.create();
        const Entry older = editor.document_.add_object("StructureRampRight", body);
        const Entry newer = editor.document_.add_object("StructureRampLeft", body);
        editor.selection_ = older; editor.refresh();
        editor.zoom_ = 1; editor.view_x_ = editor.view_y_ = 0;
        query("later-ramp-priority", { 180, 240 }, newer, "Ramp hit testing changed reverse-authored collision priority");
        const Entry rectangle = editor.document_.add_object("Structure", body);
        editor.refresh();
        query("later-rectangle-priority", { 140, 140 }, rectangle, "Ordinary rectangles lost their existing overlap priority");
        editor.document_.add_object("StructureRampRight", { 1450, 180, 160, 160 });
        const Entry spawn_a = editor.document_.add_object("SpawnA", { 1495, 300, 1, 1 });
        editor.refresh();
        query("spawn-a-priority", { 1505, 305 }, spawn_a, "Ramp selection intercepted a higher-priority Team A spawn");
        const Entry spawn_b = editor.document_.add_object("SpawnB", { 1500, 300, 1, 1 });
        editor.refresh();
        query("spawn-b-priority", { 1505, 305 }, spawn_b, "Ramp selection changed Team B versus Team A priority");
        editor.document_.add_object("SpawnA", { 1498, 300, 1, 1 }); editor.refresh();
        query("spawn-category-before-order", { 1505, 305 }, spawn_b, "Authored order displaced the existing spawn category priority");
        const Entry slab = editor.document_.add_object("FixedSlabRight", { 500, 500, 1, 1 });
        editor.refresh();
        query("slab-interior", { 585, 471 }, slab, "Ramp selection changed existing slab polygon admission");
        query("slab-empty-corner", { 505, 425 }, Entry{ EntryKind::decoration, 0, "background" },
            "Ramp selection widened the existing slab polygon hit area");

        keyboard.restore();
        editor.cancel_drag();
        editor.document_.load(original_path); editor.selection_ = original_selection; editor.refresh();
        SendMessageW(editor.window_, WM_DPICHANGED, MAKEWPARAM(original_dpi, original_dpi),
            reinterpret_cast<LPARAM>(&original_window));
        editor.zoom_ = original_zoom; editor.view_x_ = original_x; editor.view_y_ = original_y; editor.snap_ = original_snap;
        SetFocus(original_focus);
        require(GetFocus() == original_focus && !IsWindowVisible(editor.window_) && !editor.dragging_ &&
            !editor.resizing_ && !editor.panning_ && GetCapture() != editor.canvas_,
            "Ramp qualification did not restore its hidden shell/focus/gesture state");
        require(evidence.good(), "Could not complete ramp hit-test evidence");
        std::cout << "PASS native ramp selection: " << contexts << " direction/DPI/view contexts, " << queries <<
            " immutable hit queries, " << raster_samples << " actual GDI body samples, finite edge tolerances, "
            "priority, selected resize handles and exact history/source preservation\n";
    }
}
