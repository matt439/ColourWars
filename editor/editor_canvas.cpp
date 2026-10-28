#include "editor/editor.h"
#include "editor/paint_mechanism_guide.h"
#include "editor/win32_text.h"

#include <windows.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <sstream>
#include <string>

namespace colourwars_editor
{
    namespace
    {
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

    }

    void Editor::line(HDC dc, int x1, int y1, int x2, int y2, COLORREF colour,
        int width, int style) const
    {
        const LOGBRUSH brush{ BS_SOLID, colour, 0 };
        HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_FLAT | PS_JOIN_MITER | static_cast<DWORD>(style),
            static_cast<DWORD>(pixels(width)), &brush, 0, nullptr);
        HGDIOBJ previous = SelectObject(dc, pen);
        MoveToEx(dc, x1, y1, nullptr);
        LineTo(dc, x2, y2);
        SelectObject(dc, previous);
        DeleteObject(pen);
    }

    void Editor::outline(HDC dc, const RECT& rectangle, COLORREF colour, int width,
        int style) const
    {
        line(dc, rectangle.left, rectangle.top, rectangle.right, rectangle.top, colour, width, style);
        line(dc, rectangle.right, rectangle.top, rectangle.right, rectangle.bottom, colour, width, style);
        line(dc, rectangle.right, rectangle.bottom, rectangle.left, rectangle.bottom, colour, width, style);
        line(dc, rectangle.left, rectangle.bottom, rectangle.left, rectangle.top, colour, width, style);
    }

    void Editor::fit(const Rect& rectangle)
    {
        RECT client{}; GetClientRect(canvas_, &client);
        zoom_ = std::clamp(std::min((client.right - static_cast<double>(pixels(80))) / std::max(rectangle.width, 100.0),
            (client.bottom - static_cast<double>(pixels(80))) / std::max(rectangle.height, 100.0)), 0.005, 10.0);
        view_x_ = rectangle.x + rectangle.width / 2.0 - client.right / (2.0 * zoom_);
        view_y_ = rectangle.y + rectangle.height / 2.0 - client.bottom / (2.0 * zoom_);
        InvalidateRect(canvas_, nullptr, FALSE);
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
        auto motion = item.motion;
        if (motion && selected && dragging_)
        {
            try { motion = document_.motion_guide(item.entry, drag_preview_); }
            catch (const std::exception&) { motion.reset(); }
        }
        if (motion)
        {
            outline(dc, screen_rect(motion->travel_envelope), RGB(131, 190, 236), 1, PS_DASH);
            outline(dc, screen_rect(motion->standing_envelope), RGB(240, 197, 95), 1, PS_DOT);
            outline(dc, screen_rect(motion->anchor), RGB(220, 225, 230), 1, PS_DASH);
        }
        RECT screen = screen_rect(motion ? motion->initial_pose : rectangle);
        const auto value = [&item](const std::string& key) -> std::string {
            const std::map<std::string, std::string>::const_iterator found = item.values.find(key);
            return found == item.values.end() ? "" : found->second;
        };
        auto slab = item.slab;
        if (slab && selected && dragging_)
        {
            // Provisional drags can leave the bounded authoring range. Keep
            // the last valid outline; committed edits still fail atomically.
            try { slab = document_.slab_guide(item.entry, drag_preview_); }
            catch (const std::exception&) {}
        }
        if (item.entry.kind == EntryKind::bounds)
        {
            if (!show_bounds_ && !selected) return;
            const COLORREF colours[] = {RGB(239, 108, 113), RGB(131, 190, 236), RGB(204, 151, 246), RGB(240, 197, 95)};
            outline(dc, screen, colours[item.entry.index % 4], 1, PS_DASH);
            SetTextColor(dc, colours[item.entry.index % 4]);
            const std::wstring label = wide(item.entry.label);
            TextOutW(dc, screen.left + pixels(5), screen.top + pixels(4), label.c_str(), static_cast<int>(label.size()));
        }
        else if (is_spawn(item.entry))
        {
            screen = screen_rect(Rect{rectangle.x, rectangle.y,
                blockout_tuning_ ? blockout_tuning_->player_width : 0.0,
                blockout_tuning_ ? blockout_tuning_->player_height : 0.0});
            const COLORREF colour = item.entry.kind == EntryKind::spawn_a ? RGB(70, 163, 250) : RGB(255, 158, 75);
            outline(dc, screen, colour, 2);
            line(dc, screen.left - pixels(5), screen.top, screen.left + pixels(5), screen.top, colour, 2);
            line(dc, screen.left, screen.top - pixels(5), screen.left, screen.top + pixels(5), colour, 2);
            const std::wstring label = (item.entry.kind == EntryKind::spawn_a ? L"A" : L"B") + std::to_wstring(item.entry.index + 1);
            SetTextColor(dc, colour); TextOutW(dc, screen.left + pixels(3), screen.top - pixels(21), label.c_str(), static_cast<int>(label.size()));
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
            else if (slab)
            {
                POINT points[4];
                for (std::size_t index = 0; index < slab->size(); ++index)
                    points[index] = screen_point((*slab)[index].x, (*slab)[index].y);
                HBRUSH brush = CreateSolidBrush(RGB(114, 133, 149));
                HGDIOBJ old_brush = SelectObject(dc, brush), old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
                Polygon(dc, points, 4);
                SelectObject(dc, old_brush); SelectObject(dc, old_pen); DeleteObject(brush);
                for (int index = 0; index < 4; ++index)
                    line(dc, points[index].x, points[index].y, points[(index + 1) % 4].x,
                        points[(index + 1) % 4].y, RGB(40, 49, 60));
                // The thick gold line is the actual support segment; the body
                // and underside retain the collider's normal extrusion.
                line(dc, points[0].x, points[0].y, points[1].x, points[1].y, RGB(240, 197, 95), 2);
                if (!show_blockout_)
                {
                    const char* faces[] = { "top", "right", "bottom", "left" };
                    for (int index = 0; index < 4; ++index)
                        if (value(std::string("paintable_faces/") + faces[index]) == "true")
                            line(dc, points[index].x, points[index].y, points[(index + 1) % 4].x,
                                points[(index + 1) % 4].y, RGB(87, 230, 204), 2);
                }
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
                if (paintable && !show_blockout_)
                {
                    const COLORREF edge = RGB(87, 230, 204);
                    if (value("paintable_faces/left") == "true") line(dc, screen.left, screen.top, screen.left, screen.bottom, edge, 2);
                    if (value("paintable_faces/top") == "true") line(dc, screen.left, screen.top, screen.right, screen.top, edge, 2);
                    if (value("paintable_faces/right") == "true") line(dc, screen.right, screen.top, screen.right, screen.bottom, edge, 2);
                    if (value("paintable_faces/bottom") == "true") line(dc, screen.left, screen.bottom, screen.right, screen.bottom, edge, 2);
                }
            }
        }
        if (item.entry.kind == EntryKind::collision)
        {
            if (const auto guide = paint_mechanism_guide(item.values))
            {
                // The screen rectangle already follows the mover's initial phase
                // and selected drag preview. These marks never alter its envelope.
                const double scale = static_cast<double>(dpi_) / 96.0;
                TEXTMETRICW font_metrics{};
                const double line_height = GetTextMetricsW(dc, &font_metrics) ?
                    static_cast<double>(font_metrics.tmHeight) / scale : 24.0;
                PaintMechanismDrawing drawing = paint_mechanism_drawing(*guide,
                    { 0.0, 0.0, static_cast<double>(screen.right - screen.left) / scale,
                        static_cast<double>(screen.bottom - screen.top) / scale },
                    line_height);
                drawing.panel = { screen.left + drawing.panel.x * scale, screen.top + drawing.panel.y * scale,
                    drawing.panel.width * scale, drawing.panel.height * scale };
                for (MechanismStroke& stroke : drawing.strokes)
                {
                    stroke = { screen.left + stroke.x1 * scale, screen.top + stroke.y1 * scale,
                        screen.left + stroke.x2 * scale, screen.top + stroke.y2 * scale };
                }
                for (MechanismLabel& label : drawing.labels)
                { label.x = screen.left + label.x * scale; label.y = screen.top + label.y * scale; }
                const RECT panel{ static_cast<LONG>(drawing.panel.x), static_cast<LONG>(drawing.panel.y),
                    static_cast<LONG>(drawing.panel.x + drawing.panel.width),
                    static_cast<LONG>(drawing.panel.y + drawing.panel.height) };
                fill(dc, panel, RGB(36, 45, 57));
                outline(dc, panel, RGB(181, 197, 213));
                for (const MechanismStroke& stroke : drawing.strokes)
                    line(dc, static_cast<int>(stroke.x1), static_cast<int>(stroke.y1),
                        static_cast<int>(stroke.x2), static_cast<int>(stroke.y2), RGB(240, 225, 190), 2);
                SetTextColor(dc, RGB(240, 225, 190));
                for (const MechanismLabel& label : drawing.labels)
                    TextOutW(dc, static_cast<int>(label.x), static_cast<int>(label.y), label.text.c_str(),
                        static_cast<int>(label.text.size()));
            }
        }
        if (selected)
        {
            if (motion) { screen = screen_rect(motion->anchor); }
            outline(dc, screen, RGB(255, 255, 255), 2);
            if (!is_spawn(item.entry) && !slab)
                fill(dc, RECT{screen.right - pixels(5), screen.bottom - pixels(5), screen.right + pixels(6), screen.bottom + pixels(6)}, RGB(255, 255, 255));
        }
    }

    void Editor::draw_blockout(HDC dc)
    {
        if (!blockout_error_.empty())
        {
            SetTextColor(dc, RGB(255, 190, 120));
            const std::wstring text = L"Blockout aids unavailable: " + blockout_error_;
            TextOutW(dc, pixels(16), pixels(43), text.c_str(), static_cast<int>(text.size()));
            return;
        }
        for (const ScoringFace& face : scoring_faces_)
        {
            // Drag previews have not committed to the document; hide old edge
            // measurements until the geometry and diagnostics agree again.
            if (dragging_) break;
            const Rect& strip = face.strip;
            if (face.polygon)
            {
                const std::size_t a_index = face.along_x ? (face.leading ? 0 : 3) : (face.leading ? 0 : 1);
                const std::size_t b_index = face.along_x ? (face.leading ? 1 : 2) : (face.leading ? 3 : 2);
                const auto& polygon = *face.polygon;
                const POINT a = screen_point(polygon[a_index].x, polygon[a_index].y);
                const POINT b = screen_point(polygon[b_index].x, polygon[b_index].y);
                line(dc, a.x, a.y, b.x, b.y, RGB(87, 230, 204), 2);
                continue;
            }
            const double x = strip.x + (!face.along_x && !face.leading ? strip.width : 0.0);
            const double y = strip.y + (face.along_x && !face.leading ? strip.height : 0.0);
            const POINT a = screen_point(x, y);
            const POINT b = screen_point(x + (face.along_x ? strip.width : 0.0),
                y + (face.along_x ? 0.0 : strip.height));
            line(dc, a.x, a.y, b.x, b.y, RGB(87, 230, 204), 2);
        }
        std::optional<Rect> region;
        if (selection_ && selection_->kind != EntryKind::level)
            region = dragging_ ? drag_preview_ : document_.rectangle(*selection_);
        if (selection_ && !dragging_)
        {
            for (const DrawItem& item : draw_items_)
                if (same_entry(item.entry, *selection_) && item.motion) { region = item.motion->initial_pose; break; }
        }
        if (region && selection_ && is_spawn(*selection_) && blockout_tuning_)
        {
            region->width = blockout_tuning_->player_width;
            region->height = blockout_tuning_->player_height;
        }
        const bool measured = blockout_tuning_ && blockout_tuning_->measured_movement_matches;
        if (region && measured)
        {
            // A bounding guide, not a simulated ballistic curve or route promise.
            // Spawn origin is top-left; other selections launch from the top centre.
            const double x = region->x + region->width / 2.0;
            const double feet = region->y + (is_spawn(*selection_) ? region->height : 0.0);
            outline(dc, screen_rect({x - blockout_tuning_->held_travel, feet - blockout_tuning_->held_rise,
                2.0 * blockout_tuning_->held_travel, blockout_tuning_->held_rise}), RGB(240, 197, 95), 1, PS_DASH);
            outline(dc, screen_rect({x - blockout_tuning_->tap_travel, feet - blockout_tuning_->tap_rise,
                2.0 * blockout_tuning_->tap_travel, blockout_tuning_->tap_rise}), RGB(204, 151, 246), 1, PS_DOT);
        }
        std::wostringstream summary;
        summary << std::fixed; summary.precision(1);
        summary << L"Scoring area " << scoring_area(scoring_faces_) << L" units squared";
        if (region) summary << L"  |  In selection " << scoring_area(scoring_faces_, region);
        if (std::any_of(draw_items_.begin(), draw_items_.end(), [](const DrawItem& item) { return item.motion.has_value(); }))
            summary << L"  |  Movers at initial phase";
        std::wostringstream tap_label, held_label;
        if (measured)
        {
            tap_label << L"Measured at " << blockout_tuning_->measured_speed << L" units/s: tap rise "
                << blockout_tuning_->tap_rise << L" / travel " << blockout_tuning_->tap_travel << L" (purple)";
            held_label << L"Held rise " << blockout_tuning_->held_rise << L" / travel " << blockout_tuning_->held_travel
                << L" (gold). Free flight, same-height landing.";
        }
        const std::wstring lines[] = {
            dragging_ ? L"Scoring measurements update on drop" : summary.str(),
            measured ? tap_label.str()
                : L"Movement tuning changed: remeasure before using the jump guide",
            measured ? held_label.str()
                : L"Spawn sizes and scoring area still follow current source tuning.",
            L"Guide boxes start at selection; no collision test or guaranteed route. Source tuning.json."};
        RECT client{}; GetClientRect(canvas_, &client);
        fill(dc, RECT{pixels(8), pixels(38), client.right - pixels(8), pixels(128)}, RGB(28, 36, 46));
        SetTextColor(dc, RGB(181, 197, 213));
        for (int index = 0; index < 4; ++index)
            TextOutW(dc, pixels(16), pixels(43 + index * 21), lines[index].c_str(), static_cast<int>(lines[index].size()));
    }

    void Editor::paint(HDC dc, const RECT& client)
    {
        fill(dc, client, RGB(28, 36, 46));
        SetBkMode(dc, TRANSPARENT);
        HGDIOBJ old_font = SelectObject(dc, font_);
        double spacing = static_cast<double>(snap_);
        while (spacing * zoom_ < pixels(28)) spacing *= 2.0;
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
        if (show_blockout_) draw_blockout(dc);
        const std::wstring legend = L"GEOMETRY VIEW    " + std::to_wstring(static_cast<int>(std::round(zoom_ * 100.0)))
            + L"%    |    Grid " + std::to_wstring(static_cast<int>(spacing));
        fill(dc, RECT{pixels(8), pixels(8), pixels(370), pixels(35)}, RGB(28, 36, 46));
        SetTextColor(dc, RGB(181, 197, 213)); TextOutW(dc, pixels(16), pixels(13), legend.c_str(), static_cast<int>(legend.size()));
        SelectObject(dc, old_font);
    }

    std::optional<Entry> Editor::hit(POINT point) const
    {
        for (EntryKind kind : {EntryKind::spawn_b, EntryKind::spawn_a, EntryKind::collision, EntryKind::decoration})
        {
            for (std::vector<DrawItem>::const_reverse_iterator item = draw_items_.rbegin(); item != draw_items_.rend(); ++item)
            {
                if (item->entry.kind != kind) continue;
                if (item->slab)
                {
                    const double x = view_x_ + point.x / zoom_, y = view_y_ + point.y / zoom_;
                    bool inside = true;
                    for (std::size_t index = 0; index < item->slab->size(); ++index)
                    {
                        const auto& a = (*item->slab)[index];
                        const auto& b = (*item->slab)[(index + 1) % item->slab->size()];
                        if ((b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x) < 0.0)
                        { inside = false; break; }
                    }
                    if (inside) { return item->entry; }
                    continue;
                }
                RECT rectangle = screen_rect(item->rectangle);
                const auto type = item->values.find("type");
                if (kind == EntryKind::collision && type != item->values.end() && type->second == "StructureRamp")
                {
                    const auto collision = item->values.find("collision_type");
                    const bool left = collision != item->values.end() && collision->second == "STRUCTURE_RAMP_LEFT";
                    const POINT points[] = {{rectangle.left, rectangle.bottom}, {rectangle.right, rectangle.bottom},
                        {left ? rectangle.left : rectangle.right, rectangle.top}};
                    const auto cross = [](POINT a, POINT b, POINT c) {
                        return (static_cast<double>(b.x) - a.x) * (static_cast<double>(c.y) - a.y)
                            - (static_cast<double>(b.y) - a.y) * (static_cast<double>(c.x) - a.x);
                    };
                    const double area = cross(points[0], points[1], points[2]);
                    const double sides[] = {cross(points[0], points[1], point), cross(points[1], points[2], point),
                        cross(points[2], points[0], point)};
                    if (area != 0.0 && ((sides[0] >= 0.0 && sides[1] >= 0.0 && sides[2] >= 0.0)
                        || (sides[0] <= 0.0 && sides[1] <= 0.0 && sides[2] <= 0.0))) return item->entry;
                    const double tolerance = static_cast<double>(pixels(3));
                    for (int edge = 0; edge < 3; ++edge)
                    {
                        const POINT a = points[edge], b = points[(edge + 1) % 3];
                        const double dx = static_cast<double>(b.x) - a.x, dy = static_cast<double>(b.y) - a.y;
                        const double px = static_cast<double>(point.x) - a.x, py = static_cast<double>(point.y) - a.y;
                        const double length_squared = dx * dx + dy * dy;
                        // Collapsed triangles retain only their finite edges and endpoints.
                        const double along = length_squared > 0.0
                            ? std::clamp((px * dx + py * dy) / length_squared, 0.0, 1.0) : 0.0;
                        const double distance_x = px - along * dx, distance_y = py - along * dy;
                        if (distance_x * distance_x + distance_y * distance_y <= tolerance * tolerance)
                            return item->entry;
                    }
                    continue;
                }
                if (is_spawn(item->entry))
                {
                    rectangle = screen_rect(Rect{item->rectangle.x, item->rectangle.y,
                        blockout_tuning_ ? blockout_tuning_->player_width : 0.0,
                        blockout_tuning_ ? blockout_tuning_->player_height : 0.0});
                    rectangle.right = std::max(rectangle.right, rectangle.left + pixels(18));
                    rectangle.bottom = std::max(rectangle.bottom, rectangle.top + pixels(24));
                }
                InflateRect(&rectangle, pixels(3), pixels(3));
                if (PtInRect(&rectangle, point)) return item->entry;
                if (item->motion)
                {
                    rectangle = screen_rect(item->motion->initial_pose);
                    InflateRect(&rectangle, pixels(3), pixels(3));
                    if (PtInRect(&rectangle, point)) return item->entry;
                }
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
        if (dragging_ || panning_) cancel_drag();
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
            cancel_drag();
            if (selection_ && selection_->kind != EntryKind::level && !is_spawn(*selection_) &&
                !document_.slab_guide(*selection_))
            {
                const RECT rectangle = screen_rect(document_.rectangle(*selection_));
                resizing_ = std::abs(point.x - rectangle.right) <= pixels(9) && std::abs(point.y - rectangle.bottom) <= pixels(9);
            }
            if (!resizing_)
            {
                // Selected bounds can be moved by their outline without intercepting geometry.
                bool on_bound = false;
                if (selection_ && selection_->kind == EntryKind::bounds)
                {
                    RECT rectangle = screen_rect(document_.rectangle(*selection_));
                    RECT inner = rectangle;
                    InflateRect(&rectangle, pixels(5), pixels(5)); InflateRect(&inner, -pixels(5), -pixels(5));
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
        case WM_MBUTTONUP: if (panning_) cancel_drag(); return 0;
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
}
