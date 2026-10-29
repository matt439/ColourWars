#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/window_layout.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

namespace level_editor_acceptance
{
    namespace
    {
        LONG width(const RECT& rectangle) { return rectangle.right - rectangle.left; }
        LONG height(const RECT& rectangle) { return rectangle.bottom - rectangle.top; }
        bool contained(const RECT& outer, const RECT& inner)
        {
            return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right &&
                inner.bottom <= outer.bottom && width(inner) > 0 && height(inner) > 0;
        }
        bool overlaps(const RECT& first, const RECT& second)
        {
            return first.left < second.right && first.right > second.left &&
                first.top < second.bottom && first.bottom > second.top;
        }

        void check_controls(int client_width, int client_height, UINT dpi)
        {
            const EditorWindowLayout layout = editor_window_layout(client_width, client_height, dpi);
            const RECT client{ 0, 0, client_width, client_height };
            std::vector<RECT> controls(layout.toolbar.begin(), layout.toolbar.end());
            for (const RECT rectangle : { layout.palette, layout.grid, layout.object_heading,
                layout.inspector_heading, layout.object_list, layout.canvas, layout.property_list,
                layout.property_label, layout.value_edit, layout.apply, layout.revert, layout.status })
            { controls.push_back(rectangle); }
            if (layout.show_hint) { controls.push_back(layout.hint); }
            for (std::size_t index = 0; index < controls.size(); ++index)
            {
                require(contained(client, controls[index]), "DPI layout put a control outside the client area");
                for (std::size_t other = index + 1; other < controls.size(); ++other)
                {
                    require(!overlaps(controls[index], controls[other]),
                        "DPI layout overlapped controls, a hint or the status line");
                }
            }
            const auto scaled = [dpi](int value) { return MulDiv(value, static_cast<int>(dpi), 96); };
            require(height(layout.property_label) == scaled(24) && height(layout.value_edit) == scaled(28) &&
                height(layout.apply) == scaled(30) && height(layout.revert) == scaled(30),
                "Compact layout shrank property labels, value editing or action heights");
            require(width(layout.apply) == scaled(94) && width(layout.revert) == scaled(94) &&
                width(layout.property_list) >= scaled(216) && height(layout.property_list) >= scaled(38),
                "DPI layout left the property list or Apply/Revert unusable");
            require(width(layout.canvas) >= scaled(200) && height(layout.canvas) >= scaled(100),
                "Responsive DPI layout lost the useful canvas");
            require(layout.combo_drop_height == scaled(220), "Combo list depth did not scale with the font");
            if (client_width <= scaled(960))
            {
                require(layout.grid.top > layout.toolbar.front().top,
                    "Narrow work-area layout failed to wrap its toolbar");
            }
        }
    }

    void check_editor_window_layout(const fs::path&)
    {
        for (const UINT dpi : { 96u, 144u, 192u })
        {
            const int scale = static_cast<int>(dpi);
            require(editor_pixels(16, dpi) == MulDiv(16, scale, 96) &&
                editor_pixels(-16, dpi) == MulDiv(-16, scale, 96),
                "Editor pixel scaling changed the signed MulDiv font/layout rule");
            for (const RECT work : { RECT{ -1920, 40, 1920, 2200 }, RECT{ -1280, 0, 0, 720 } })
            {
                const RECT initial = initial_editor_rectangle(dpi, work);
                const SIZE minimum = minimum_editor_size(dpi, work);
                require(contained(work, initial), "Initial editor rectangle escaped a monitor work area");
                require(width(initial) == std::min(MulDiv(1500, scale, 96), static_cast<int>(width(work))) &&
                    height(initial) == std::min(MulDiv(940, scale, 96), static_cast<int>(height(work))),
                    "Initial editor outer size did not scale and clamp to the work area");
                require(initial.left - work.left == (width(work) - width(initial)) / 2 &&
                    initial.top - work.top == (height(work) - height(initial)) / 2,
                    "Initial editor was not centered in the selected work area");
                require(minimum.cx == std::min(MulDiv(1150, scale, 96), static_cast<int>(width(work))) &&
                    minimum.cy == std::min(MulDiv(720, scale, 96), static_cast<int>(height(work))) &&
                    minimum.cx <= width(initial) && minimum.cy <= height(initial),
                    "Minimum tracking size exceeded the available work area or initial size");
            }
            for (const SIZE logical : { SIZE{ 620, 280 }, SIZE{ 620, 300 }, SIZE{ 960, 500 }, SIZE{ 1500, 900 } })
            {
                check_controls(MulDiv(logical.cx, scale, 96), MulDiv(logical.cy, scale, 96), dpi);
            }
            // A real 1280x720 work-area clamp at 192 DPI has fewer logical
            // pixels; leave room for a conservative native frame/menu extent.
            check_controls(1280 - MulDiv(16, scale, 96), 720 - MulDiv(70, scale, 96), dpi);
        }
        std::cout << "PASS editor DPI layout: 96/144/192 font scaling, centered work-area clamps, "
            "minimums, toolbar wrapping, contained controls and reachable property actions\n";
    }
}
