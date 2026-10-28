#include "editor/window_dpi.h"
#include <shellscalingapi.h>

#include <bit>
#include <system_error>

namespace colourwars_editor
{
    namespace
    {
        template<typename Function> Function user_function(const char* name)
        {
            return std::bit_cast<Function>(GetProcAddress(GetModuleHandleW(L"user32.dll"), name));
        }
    }

    void enable_editor_dpi_awareness()
    {
        const auto set_context = user_function<decltype(&SetProcessDpiAwarenessContext)>("SetProcessDpiAwarenessContext");
        if (set_context)
        {
            if (set_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) || GetLastError() == ERROR_ACCESS_DENIED)
            { return; }
            if (set_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE) || GetLastError() == ERROR_ACCESS_DENIED)
            { return; }
        }
        const HRESULT result = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        if (SUCCEEDED(result) || result == E_ACCESSDENIED) { return; }
        throw std::system_error(static_cast<int>(result), std::system_category(),
            "Could not set the editor's per-monitor DPI awareness");
    }

    UINT editor_monitor_dpi(HMONITOR monitor)
    {
        UINT x = 96, y = 96;
        if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &x, &y)) && x > 0) { return x; }
        HDC screen = GetDC(nullptr);
        const int value = screen ? GetDeviceCaps(screen, LOGPIXELSX) : 0;
        if (screen) { ReleaseDC(nullptr, screen); }
        return value > 0 ? static_cast<UINT>(value) : 96u;
    }

    UINT editor_window_dpi(HWND window)
    {
        const auto get_dpi = user_function<decltype(&GetDpiForWindow)>("GetDpiForWindow");
        const UINT dpi = get_dpi ? get_dpi(window) : 0;
        return dpi > 0 ? dpi : editor_monitor_dpi(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST));
    }

    RECT editor_monitor_work_area(HMONITOR monitor)
    {
        MONITORINFO information{ sizeof(information) };
        if (!GetMonitorInfoW(monitor, &information))
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                "Could not read the editor monitor's work area");
        }
        return information.rcWork;
    }
}
