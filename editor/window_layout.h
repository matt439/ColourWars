#pragma once

#include <windows.h>
#include <array>

namespace colourwars_editor
{
    int editor_pixels(int logical, UINT dpi);
    RECT initial_editor_rectangle(UINT dpi, const RECT& work);
    SIZE minimum_editor_size(UINT dpi, const RECT& work);

    // Coordinates are client pixels at `dpi`. Combo rectangles describe their
    // closed controls; combo_drop_height is additional space for the open list.
    struct EditorWindowLayout
    {
        std::array<RECT, 8> toolbar{};
        RECT palette{};
        RECT grid{};
        RECT object_heading{};
        RECT inspector_heading{};
        RECT object_list{};
        RECT canvas{};
        RECT property_list{};
        RECT property_label{};
        RECT value_edit{};
        RECT apply{};
        RECT revert{};
        RECT hint{};
        RECT status{};
        bool show_hint = false;
        int combo_drop_height = 0;
    };

    EditorWindowLayout editor_window_layout(int width, int height, UINT dpi);
}
