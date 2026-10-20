// ENG#17: hidden per-monitor-v2 baseline plus pure DPI rectangle arithmetic.
// No display-setting change, monitor move, fake DPI message, show or state loop.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine/app/application.h"

#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    void print_rect(const char* kind, UINT dpi, const RECT& rect)
    {
        std::cout << "RECT_MATH kind=" << kind << " dpi=" << dpi
            << " outer=" << rect.right - rect.left << 'x' << rect.bottom - rect.top
            << " offsets=" << rect.left << ',' << rect.top << ','
            << rect.right << ',' << rect.bottom << '\n';
    }
}

int main()
{
    try
    {
        HWND window = nullptr;
        {
            labrador::ApplicationOptions options;
            options.window_class_name = L"DpiWindowBaseline_" +
                std::to_wstring(GetCurrentProcessId());
            options.window_title = L"Hidden DPI baseline";
            options.visible = false;
            options.fullscreen = false;
            options.resolution = labrador::ScreenResolution::s_1280_720;
            options.min_threads = options.max_threads = options.view_capacity = 1;
            labrador::Application app(options);
            app.initialize(); // Real D3D11/XAudio2 services; no assets/state/sound.
            window = static_cast<HWND>(app.window());
            DWORD owner = 0;
            require(window != nullptr && IsWindow(window) != FALSE &&
                GetWindowThreadProcessId(window, &owner) == GetCurrentThreadId() &&
                owner == GetCurrentProcessId(), "The live HWND is not owned by this thread");
            require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window,
                "The fixture must remain hidden and nonforeground");
            require(AreDpiAwarenessContextsEqual(GetWindowDpiAwarenessContext(window),
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE,
                "Embed dpi_window_baseline.manifest before running this fixture");

            const UINT dpi = GetDpiForWindow(window);
            require(dpi != 0, "GetDpiForWindow failed");
            RECT client{};
            require(GetClientRect(window, &client) != FALSE,
                "GetClientRect failed for the owned window");
            const auto layout = app.resolution_manager()->resolution_ivec();
            const auto buffer = app.renderer()->back_buffer_size();
            const LONG client_width = client.right - client.left;
            const LONG client_height = client.bottom - client.top;
            std::cout << "HIDDEN_DPI_BASELINE awareness=per-monitor-v2 dpi=" << dpi
                << " requested=1280x720 client=" << client_width << 'x' << client_height
                << " client_matches_requested=" <<
                    (client_width == 1280 && client_height == 720)
                << " layout=" << layout.x << 'x' << layout.y
                << " back_buffer=" << buffer.x << 'x' << buffer.y
                << " hidden=1 foreground=0 assets_loaded=0 run_calls=0\n";

            const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(window, GWL_STYLE));
            const DWORD ex_style = static_cast<DWORD>(GetWindowLongPtrW(window, GWL_EXSTYLE));
            require((style & WS_OVERLAPPEDWINDOW) == WS_OVERLAPPEDWINDOW &&
                GetMenu(window) == nullptr, "Expected the ordinary no-menu engine window");
            RECT legacy{0, 0, 1280, 720};
            require(AdjustWindowRectEx(&legacy, style, FALSE, ex_style) != FALSE,
                "Legacy rectangle arithmetic failed");
            // This independent arithmetic call mirrors the API named by the
            // current engine source. It is not a DPI-aware sizing recommendation.
            print_rect("legacy", 0, legacy);
            for (const UINT calculation_dpi : std::array<UINT, 4>{96, 120, 144, 192})
            {
                RECT scaled{0, 0, 1280, 720};
                require(AdjustWindowRectExForDpi(&scaled, style, FALSE, ex_style,
                    calculation_dpi) != FALSE, "DPI-explicit rectangle arithmetic failed");
                require(scaled.right > scaled.left && scaled.bottom > scaled.top,
                    "DPI-explicit rectangle has no extent");
                print_rect("explicit", calculation_dpi, scaled);
            }
            require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window,
                "The query-only fixture lost hidden/nonforeground isolation");
            // Scope teardown destroys the window without pumping queued events.
        }
        require(IsWindow(window) == FALSE, "Scope teardown left the owned HWND alive");
        std::cout << "PASS dpi baseline and rectangle arithmetic; scaled_monitor_tested=0"
            << " dpi_transition_tested=0 pixels_tested=0\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "DPI baseline failed: " << error.what() << '\n';
        return 1;
    }
}
