#include "game/pch.h"
#include "game/config/initial_display.h"
#include "game/save/save.h"
#include "engine/app/application.h"

InitialDisplay choose_initial_display(int work_width, int work_height,
    int frame_width, int frame_height)
{
    using labrador::ScreenResolution;
    const int client_width = work_width - std::max(0, frame_width);
    const int client_height = work_height - std::max(0, frame_height);
    if (client_width >= 1920 && client_height >= 1080)
    {
        return { ScreenResolution::s_1920_1080, false };
    }
    if (client_width >= 1280 && client_height >= 720)
    {
        return { ScreenResolution::s_1280_720, false };
    }
    // The engine's smallest preset will not fit as a window. Borderless
    // fullscreen fits the monitor and scales the same menu design into it.
    return { ScreenResolution::s_1280_720, true };
}

InitialDisplay initial_display_for_primary_monitor()
{
    // The executable is per-monitor v2 aware; hold that context explicitly so
    // this query also has physical-pixel semantics in a console probe.
    const DPI_AWARENESS_CONTEXT previous = SetThreadDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    const bool available = GetMonitorInfoW(MonitorFromPoint(POINT{ 0, 0 },
        MONITOR_DEFAULTTOPRIMARY), &monitor) != FALSE;
    RECT frame{};
    const bool measured = AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW,
        FALSE, 0, GetDpiForSystem()) != FALSE;
    if (previous != nullptr) { SetThreadDpiAwarenessContext(previous); }
    if (!available || !measured)
    {
        return { labrador::ScreenResolution::s_1280_720, true };
    }
    return choose_initial_display(monitor.rcWork.right - monitor.rcWork.left,
        monitor.rcWork.bottom - monitor.rcWork.top, frame.right - frame.left,
        frame.bottom - frame.top);
}

void fit_initial_window(labrador::Application& app, Save& save)
{
    if (!save.first_run()) { return; }
    const HWND window = static_cast<HWND>(app.window());
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    RECT frame{};
    if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor) == FALSE ||
        AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0, GetDpiForWindow(window)) == FALSE)
    {
        return;
    }
    const InitialDisplay display = choose_initial_display(
        monitor.rcWork.right - monitor.rcWork.left, monitor.rcWork.bottom - monitor.rcWork.top,
        frame.right - frame.left, frame.bottom - frame.top);
    if (save.resolution() != display.resolution) { app.set_resolution(display.resolution); }
    if (app.fullscreen() != display.fullscreen) { app.set_fullscreen(display.fullscreen); }
    if (!display.fullscreen)
    {
        RECT outer{};
        if (GetWindowRect(window, &outer) != FALSE)
        {
            const int x = monitor.rcWork.left +
                (monitor.rcWork.right - monitor.rcWork.left - (outer.right - outer.left)) / 2;
            const int y = monitor.rcWork.top +
                (monitor.rcWork.bottom - monitor.rcWork.top - (outer.bottom - outer.top)) / 2;
            SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    if (save.resolution() != display.resolution || save.fullscreen() != display.fullscreen)
    {
        save.set_display_settings_and_save(display.resolution, display.fullscreen, save.team_palette());
    }
}
