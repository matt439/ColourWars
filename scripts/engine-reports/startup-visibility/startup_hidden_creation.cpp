// ENG#19 positive control: initial hidden creation already exists.
// Source preparation only. No assets, state loop, drawing, show call or save.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine/app/application.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }
}

int wmain(int argc, wchar_t** argv)
{
    if (argc > 2 || (argc == 2 && std::wstring_view(argv[1]) != L"--fullscreen"))
    {
        std::cerr << "usage: startup_hidden_creation.exe [--fullscreen]\n";
        return 2;
    }
    try
    {
        HWND handle = nullptr;
        const bool fullscreen = argc == 2;
        {
            labrador::ApplicationOptions options;
            options.window_class_name = L"LabradorStartupHiddenExhibit";
            options.window_title = L"Hidden startup API positive control";
            options.visible = false;
            options.fullscreen = fullscreen;
            options.resolution = labrador::ScreenResolution::s_1280_720;
            options.min_threads = 1;
            options.max_threads = 1;
            options.view_capacity = 1;
            labrador::Application app(options);
            app.initialize();
            handle = static_cast<HWND>(app.window());
            require(handle != nullptr && IsWindow(handle) != FALSE,
                "initialize did not leave a live native window");
            DWORD owner = 0;
            require(GetWindowThreadProcessId(handle, &owner) != 0 &&
                owner == GetCurrentProcessId(), "window is not owned by this process");
            require(IsWindowVisible(handle) == FALSE,
                "visible=false created a shown window");
            RECT client{};
            require(GetClientRect(handle, &client) != FALSE,
                "could not query owned window client rectangle");
            const auto layout = app.resolution_manager()->resolution_ivec();
            const auto back_buffer = app.renderer()->back_buffer_size();
            const auto& device = app.renderer()->device_info();
            std::cout << "HIDDEN_INITIALIZE fullscreen=" << fullscreen
                << " owned=1 visible=0 assets_loaded=0 run_calls=0"
                << " client=" << client.right - client.left << 'x'
                << client.bottom - client.top
                << " layout=" << layout.x << 'x' << layout.y
                << " back_buffer=" << back_buffer.x << 'x' << back_buffer.y
                << " backend=" << device.backend << " api=" << device.api << '\n';
            // No quit(): scope teardown detaches notifications and destroys the
            // window without leaving the ordinary close path's queued WM_QUIT.
        }
        require(IsWindow(handle) == FALSE, "scope teardown left its window alive");
        std::cout << "PASS hidden_creation_and_teardown=1 later_show_tested=0"
            << " first_present_tested=0\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "hidden creation exhibit failed: " << error.what() << '\n';
        return 1;
    }
}
