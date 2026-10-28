#pragma once

#include <windows.h>

namespace colourwars_editor
{
    // Called before creating editor UI. Older Windows 10 builds fall back to
    // per-monitor v1 when the v2 setter is unavailable or unsupported.
    void enable_editor_dpi_awareness();
    UINT editor_monitor_dpi(HMONITOR monitor);
    UINT editor_window_dpi(HWND window);
    RECT editor_monitor_work_area(HMONITOR monitor);
}
