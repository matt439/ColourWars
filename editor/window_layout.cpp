#include "editor/window_layout.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace colourwars_editor
{
    namespace
    {
        RECT box(int x, int y, int width, int height)
        {
            return { x, y, x + std::max(1, width), y + std::max(1, height) };
        }
    }

    int editor_pixels(int logical, UINT dpi)
    {
        if (dpi == 0 || dpi > static_cast<UINT>((std::numeric_limits<int>::max)()))
        { throw std::invalid_argument("Editor DPI must be a positive supported integer."); }
        return MulDiv(logical, static_cast<int>(dpi), 96);
    }

    RECT initial_editor_rectangle(UINT dpi, const RECT& work)
    {
        const int available_width = static_cast<int>(std::max(1L, work.right - work.left));
        const int available_height = static_cast<int>(std::max(1L, work.bottom - work.top));
        const int width = std::min(editor_pixels(1500, dpi), available_width);
        const int height = std::min(editor_pixels(940, dpi), available_height);
        return box(static_cast<int>(work.left) + (available_width - width) / 2,
            static_cast<int>(work.top) + (available_height - height) / 2, width, height);
    }

    SIZE minimum_editor_size(UINT dpi, const RECT& work)
    {
        return { std::min(editor_pixels(1150, dpi), static_cast<int>(std::max(1L, work.right - work.left))),
            std::min(editor_pixels(720, dpi), static_cast<int>(std::max(1L, work.bottom - work.top))) };
    }

    EditorWindowLayout editor_window_layout(int width, int height, UINT dpi)
    {
        const auto pixels = [dpi](int logical) { return editor_pixels(logical, dpi); };
        EditorWindowLayout result;
        const int margin = pixels(10), gap = pixels(6), button_height = pixels(30);
        int x = margin, y = margin;
        const auto toolbar_control = [&](int logical_width)
        {
            const int control_width = std::min(pixels(logical_width), std::max(1, width - margin * 2));
            if (x > margin && x + control_width > width - margin)
            {
                x = margin;
                y += button_height + gap;
            }
            const RECT rectangle = box(x, y, control_width, button_height);
            x += control_width + gap;
            return rectangle;
        };
        for (std::size_t index = 0; index < 5; ++index) result.toolbar[index] = toolbar_control(66);
        result.palette = toolbar_control(205);
        result.toolbar[5] = toolbar_control(70);
        result.toolbar[6] = toolbar_control(100);
        result.toolbar[7] = toolbar_control(85);
        result.grid = toolbar_control(100);
        result.combo_drop_height = pixels(220);

        // On a work-area-clamped high-DPI window, the toolbar wraps and the side
        // panes narrow before the canvas or the property actions are sacrificed.
        const int left = std::min(pixels(255), std::max(pixels(140), width * 22 / 100));
        const int right = std::min(pixels(335), std::max(pixels(240), width * 29 / 100));
        const bool compact_top = height - (y + button_height) < pixels(300);
        const int canvas_top = y + button_height + pixels(compact_top ? 6 : 11);
        const int heading_gap = pixels(compact_top ? 4 : 8);
        const int content_top = canvas_top + pixels(compact_top ? 28 : 35);
        const int content_bottom = height - pixels(38);
        const int panel_x = width - right + pixels(12);
        const int panel_width = right - pixels(24);
        const int panel_height = content_bottom - content_top;
        result.object_heading = box(pixels(12), canvas_top + heading_gap, left - pixels(22), pixels(22));
        result.inspector_heading = box(panel_x, canvas_top + heading_gap, panel_width, pixels(22));
        result.object_list = box(margin, content_top, left - pixels(20), panel_height);
        result.canvas = box(left, canvas_top, width - left - right, content_bottom - canvas_top);

        const bool compact = panel_height < pixels(220);
        const int label_gap = pixels(compact ? 3 : 12);
        const int edit_gap = pixels(compact ? 2 : 4);
        const int action_gap = pixels(compact ? 3 : 9);
        const int label_height = pixels(24), edit_height = pixels(28), action_height = pixels(30);
        const int field_height = label_gap + label_height + edit_gap + edit_height + action_gap + action_height;
        const int minimum_list = pixels(120), hint_gap = pixels(12), minimum_hint = pixels(45);
        result.show_hint = panel_height >= field_height + minimum_list + hint_gap + minimum_hint;
        const int list_height = result.show_hint ?
            std::max(minimum_list, panel_height - field_height - hint_gap - pixels(190)) :
            panel_height - field_height;
        result.property_list = box(panel_x, content_top, panel_width, list_height);
        const int label_y = content_top + list_height + label_gap;
        result.property_label = box(panel_x, label_y, panel_width, label_height);
        const int edit_y = label_y + label_height + edit_gap;
        result.value_edit = box(panel_x, edit_y, panel_width, edit_height);
        const int action_y = edit_y + edit_height + action_gap;
        result.apply = box(panel_x, action_y, pixels(94), action_height);
        result.revert = box(panel_x + pixels(102), action_y, pixels(94), action_height);
        if (result.show_hint)
        {
            const int hint_y = action_y + action_height + hint_gap;
            result.hint = box(panel_x, hint_y, panel_width, content_bottom - hint_y);
        }
        result.status = box(pixels(12), height - pixels(26), width - pixels(24), pixels(24));
        return result;
    }
}
