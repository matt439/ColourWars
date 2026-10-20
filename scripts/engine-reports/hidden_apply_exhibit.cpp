// Synthetic engine/Win32 exhibit. No game source, content or global input.
// This program reproduces a visibility bug while keeping its own window offscreen.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "engine/app/application.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    class HideOwnedWindow
    {
    public:
        explicit HideOwnedWindow(HWND window) : window_(window) {}
        ~HideOwnedWindow()
        {
            if (IsWindow(this->window_) != FALSE) { ShowWindow(this->window_, SW_HIDE); }
        }
        HideOwnedWindow(const HideOwnedWindow&) = delete;
        HideOwnedWindow& operator=(const HideOwnedWindow&) = delete;
    private:
        HWND window_;
    };

    // WH_CBT is restricted to this thread, and only this owned HWND is denied
    // activation. Other processes/windows and the engine's visibility call are
    // untouched. A veto prevents SW_SHOWNORMAL from taking foreground.
    class PreventOwnedActivation
    {
    public:
        explicit PreventOwnedActivation(HWND window)
        {
            require(target_ == nullptr, "Activation guards must not nest");
            target_ = window;
            this->hook_ = SetWindowsHookExW(WH_CBT, &PreventOwnedActivation::filter,
                nullptr, GetCurrentThreadId());
            if (this->hook_ == nullptr)
            {
                target_ = nullptr;
                throw std::runtime_error("Cannot protect the owned window from activation");
            }
        }
        ~PreventOwnedActivation()
        {
            UnhookWindowsHookEx(this->hook_);
            target_ = nullptr;
        }
        PreventOwnedActivation(const PreventOwnedActivation&) = delete;
        PreventOwnedActivation& operator=(const PreventOwnedActivation&) = delete;
    private:
        static LRESULT CALLBACK filter(int code, WPARAM word, LPARAM parameter)
        {
            if (code == HCBT_ACTIVATE && reinterpret_cast<HWND>(word) == target_) { return 1; }
            return CallNextHookEx(nullptr, code, word, parameter);
        }
        inline static thread_local HWND target_ = nullptr;
        HHOOK hook_ = nullptr;
    };

    struct DesktopBounds
    {
        std::int64_t left, top, right, bottom;
    };

    DesktopBounds virtual_desktop()
    {
        const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        require(width > 0 && height > 0, "Cannot determine the virtual desktop bounds");
        const std::int64_t left = GetSystemMetrics(SM_XVIRTUALSCREEN);
        const std::int64_t top = GetSystemMetrics(SM_YVIRTUALSCREEN);
        return { left, top, left + width, top + height };
    }

    bool outside_desktop(const RECT& rectangle, const DesktopBounds& desktop)
    {
        return rectangle.right <= desktop.left || rectangle.left >= desktop.right ||
            rectangle.bottom <= desktop.top || rectangle.top >= desktop.bottom;
    }

    void move_owned_window_offscreen(HWND window, const DesktopBounds& desktop)
    {
        RECT rectangle{};
        require(GetWindowRect(window, &rectangle) != FALSE, "Cannot read the owned window bounds");
        const std::int64_t width = static_cast<std::int64_t>(rectangle.right) - rectangle.left;
        const std::int64_t height = static_cast<std::int64_t>(rectangle.bottom) - rectangle.top;
        require(width > 0 && height > 0, "Owned window has invalid bounds");
        const std::int64_t x = desktop.left - std::max<std::int64_t>(width, 1280) - 1024;
        const std::int64_t y = desktop.top - std::max<std::int64_t>(height, 720) - 1024;
        require(x >= std::numeric_limits<int>::min() && y >= std::numeric_limits<int>::min(),
            "Cannot represent a safe offscreen position");
        require(SetWindowPos(window, nullptr, static_cast<int>(x), static_cast<int>(y), 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE,
            "Cannot move the owned window offscreen");
        require(GetWindowRect(window, &rectangle) != FALSE && outside_desktop(rectangle, desktop),
            "Owned window placement intersects the virtual desktop");
    }
}

int main()
{
    try
    {
        labrador::ApplicationOptions options;
        options.window_class_name = L"HiddenApplyExhibit_" + std::to_wstring(GetCurrentProcessId());
        options.window_title = L"Synthetic hidden Apply exhibit";
        options.resolution = labrador::ScreenResolution::s_1280_720;
        options.fullscreen = false;
        options.visible = false;
        options.min_threads = options.max_threads = options.view_capacity = 1;
        labrador::Application app(options);
        app.initialize(); // Actual engine shell; no manifest, content or state.

        const HWND window = static_cast<HWND>(app.window());
        HideOwnedWindow cleanup(window);
        DWORD owner = 0;
        require(GetWindowThreadProcessId(window, &owner) == GetCurrentThreadId() &&
            owner == GetCurrentProcessId(), "The target is not this thread's owned window");
        require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window,
            "The engine did not initialize an inactive hidden window");
        PreventOwnedActivation activation(window);
        const DesktopBounds desktop = virtual_desktop();
        move_owned_window_offscreen(window, desktop);

        const bool before_visible = IsWindowVisible(window) != FALSE;
        app.set_fullscreen(false); // The ordinary already-windowed Apply path.
        const bool after_visible = IsWindowVisible(window) != FALSE;
        const bool became_foreground = GetForegroundWindow() == window;
        RECT after_rectangle{};
        const bool read_rectangle = GetWindowRect(window, &after_rectangle) != FALSE;
        ShowWindow(window, SW_HIDE); // Hide before reporting or throwing.

        require(!before_visible && !app.fullscreen(), "The exhibit changed its windowed setup");
        require(!became_foreground && GetForegroundWindow() != window,
            "The owned window unexpectedly became foreground");
        require(read_rectangle && outside_desktop(after_rectangle, desktop),
            "Apply moved the owned window onto the virtual desktop");
        require(IsWindowVisible(window) == FALSE, "Cannot hide the owned window after observation");
        if (!after_visible)
        {
            std::cout << "NOT_REPRODUCED hidden Apply: before_visible=0 after_visible=0 final_visible=0\n";
            return 2; // Recheck/withdraw the bug if a newer engine preserves hidden state.
        }
        std::cout << "REPRODUCED hidden Apply: before_visible=0 after_visible=1 final_visible=0 offscreen=1 foreground=0\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
