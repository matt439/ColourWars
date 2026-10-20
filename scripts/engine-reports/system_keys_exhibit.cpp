// Synthetic owned-HWND system-key exhibit. No game source, content or global input.
// Every SC_KEYMENU request is intercepted before the engine/default menu handler.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "engine/app/application.h"

#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>
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

    enum class Phase : std::size_t { guard, ordinary_a, f10, bare_alt, count };
    struct Observation
    {
        unsigned int down_messages = 0, up_messages = 0;
        bool raw_down = false, raw_up = false;
        unsigned int menu_requests = 0, zero_character_requests = 0;
    };

    // This object is declared before Application and therefore outlives its
    // HWND on every unwind path. A failed detach cannot leave a dangling
    // callback context while Application destroys the window.
    class SystemMenuTrap
    {
    public:
        explicit SystemMenuTrap(HWND window) : window_(window), thread_(GetCurrentThreadId())
        {
            require(current_ == nullptr, "Owned WndProc taps must not nest");
            require(this->owned(), "WndProc tap target is not this thread's owned window");
            this->previous_ = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
            require(this->previous_ != nullptr, "Cannot read the engine WndProc");
            current_ = this;
            const LONG_PTR old = SetWindowLongPtrW(window, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(&SystemMenuTrap::dispatch));
            if (old == 0)
            {
                current_ = nullptr;
                throw std::runtime_error("Cannot install the owned system-menu barrier");
            }
            this->previous_ = reinterpret_cast<WNDPROC>(old);
            this->attached_ = true;
        }
        ~SystemMenuTrap()
        {
            // Application normally destroyed this HWND first. Check ownership
            // again before any fallback restoration, including handle reuse.
            if (this->attached_ && this->owned() &&
                GetWindowLongPtrW(this->window_, GWLP_WNDPROC) ==
                    reinterpret_cast<LONG_PTR>(&SystemMenuTrap::dispatch))
            {
                SetWindowLongPtrW(this->window_, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(this->previous_));
            }
            current_ = nullptr;
        }
        SystemMenuTrap(const SystemMenuTrap&) = delete;
        SystemMenuTrap& operator=(const SystemMenuTrap&) = delete;

        void phase(Phase value) { this->phase_ = value; }
        Observation& observation(Phase value)
        {
            return this->observations_.at(static_cast<std::size_t>(value));
        }
        bool entered_menu_loop() const { return this->entered_menu_loop_; }
        bool installed() const
        {
            return this->owned() && GetWindowLongPtrW(this->window_, GWLP_WNDPROC) ==
                reinterpret_cast<LONG_PTR>(&SystemMenuTrap::dispatch);
        }
        bool detach()
        {
            if (!this->attached_) { return true; }
            if (!this->installed()) { return false; }
            const LONG_PTR old = SetWindowLongPtrW(this->window_, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(this->previous_));
            if (old == 0) { return false; }
            this->attached_ = false;
            current_ = nullptr;
            return true;
        }
    private:
        bool owned() const
        {
            DWORD process = 0;
            return GetWindowThreadProcessId(this->window_, &process) == this->thread_ &&
                process == GetCurrentProcessId();
        }
        static LRESULT CALLBACK dispatch(HWND window, UINT message, WPARAM word, LPARAM parameter)
        {
            SystemMenuTrap* self = current_;
            // This procedure is installed only on the owned HWND. If an
            // unexpected callback has no live context, fail closed rather than
            // passing any possible menu command to DefWindowProc.
            if (self == nullptr || self->window_ != window) { return 0; }
            Observation& result = self->observations_[static_cast<std::size_t>(self->phase_)];
            if (message == WM_SYSCOMMAND && (word & 0xFFF0u) == static_cast<WPARAM>(SC_KEYMENU))
            {
                ++result.menu_requests;
                if (parameter == 0) { ++result.zero_character_requests; }
                return 0; // Critical barrier: never chain SC_KEYMENU to the engine/default proc.
            }
            if (message == WM_ENTERMENULOOP)
            {
                self->entered_menu_loop_ = true;
                return 0;
            }
            if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) { ++result.down_messages; }
            if (message == WM_KEYUP || message == WM_SYSKEYUP) { ++result.up_messages; }
            return CallWindowProcW(self->previous_, window, message, word, parameter);
        }
        inline static thread_local SystemMenuTrap* current_ = nullptr;
        HWND window_;
        DWORD thread_;
        WNDPROC previous_ = nullptr;
        bool attached_ = false, entered_menu_loop_ = false;
        Phase phase_ = Phase::guard;
        std::array<Observation, static_cast<std::size_t>(Phase::count)> observations_{};
    };

    constexpr LPARAM key_parameter(std::uint32_t scan, bool alt_down, bool released)
    {
        std::uint32_t bits = 1u | (scan << 16); // One event; set-1 scan position.
        if (alt_down) { bits |= 1u << 29; }
        if (released) { bits |= (1u << 30) | (1u << 31); }
        return static_cast<LPARAM>(bits);
    }
    static_assert(key_parameter(0x44, false, false) == 0x00440001LL);
    static_assert(key_parameter(0x44, false, true) == 0xC0440001LL);
    static_assert(key_parameter(0x38, true, false) == 0x20380001LL);
    static_assert(key_parameter(0x38, false, true) == 0xC0380001LL);
    using KeyCallback = void (labrador::WindowNotify::*)(labrador::Key) const;
    static_assert(std::is_same_v<decltype(&labrador::WindowNotify::on_key_down), KeyCallback>);
    static_assert(std::is_same_v<decltype(&labrador::WindowNotify::on_key_up), KeyCallback>);

    void still_isolated(HWND window, const DesktopBounds& desktop)
    {
        RECT rectangle{};
        require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window &&
            GetWindowRect(window, &rectangle) != FALSE && outside_desktop(rectangle, desktop),
            "The owned window lost hidden/offscreen/nonforeground isolation");
    }

    void send_pair(labrador::Application& app, SystemMenuTrap& trap, Phase phase,
        UINT down, UINT up, WPARAM virtual_key, std::uint32_t scan, labrador::Key key,
        bool system_alt, const DesktopBounds& desktop, bool initial_logical_focus)
    {
        const HWND window = static_cast<HWND>(app.window());
        require(trap.installed(), "System-menu barrier is not installed before key stimuli");
        require(app.keyboard()->focused() == initial_logical_focus,
            "Engine logical keyboard focus changed before the owned key pair");
        trap.phase(phase);
        SendMessageW(window, down, virtual_key, key_parameter(scan, system_alt, false));
        app.keyboard()->poll();
        require(app.keyboard()->focused() == initial_logical_focus,
            "Engine logical keyboard focus changed after the owned key-down");
        Observation& result = trap.observation(phase);
        result.raw_down = app.keyboard()->state().is_down(key);
        still_isolated(window, desktop);
        SendMessageW(window, up, virtual_key, key_parameter(scan, false, true));
        app.keyboard()->poll();
        require(app.keyboard()->focused() == initial_logical_focus,
            "Engine logical keyboard focus changed after the owned key-up");
        result.raw_up = !app.keyboard()->state().is_down(key);
        require(result.down_messages == 1 && result.up_messages == 1 && result.raw_down && result.raw_up,
            "Owned key messages did not reach the real engine key snapshots exactly once");
        still_isolated(window, desktop);
    }
}

int main()
{
    try
    {
        std::unique_ptr<SystemMenuTrap> trap; // Must outlive Application on failed-detach paths.
        labrador::ApplicationOptions options;
        options.window_class_name = L"SystemKeysExhibit_" + std::to_wstring(GetCurrentProcessId());
        options.window_title = L"Synthetic system-key request exhibit";
        options.fullscreen = false;
        options.visible = false;
        options.min_threads = options.max_threads = options.view_capacity = 1;
        labrador::Application app(options);
        app.initialize(); // Actual shell; no manifest/content/state and no played sound.
        const HWND window = static_cast<HWND>(app.window());
        HideOwnedWindow cleanup(window);
        DWORD owner = 0;
        require(GetWindowThreadProcessId(window, &owner) == GetCurrentThreadId() &&
            owner == GetCurrentProcessId() && GetAncestor(window, GA_ROOT) == window &&
            GetWindow(window, GW_OWNER) == nullptr, "The target is not this thread's unowned top-level HWND");
        require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window,
            "The shell did not initialize hidden/nonforeground");
        PreventOwnedActivation activation(window);
        const DesktopBounds desktop = virtual_desktop();
        move_owned_window_offscreen(window, desktop);
        const LONG_PTR engine_user_data = GetWindowLongPtrW(window, GWLP_USERDATA);
        trap = std::make_unique<SystemMenuTrap>(window);
        require(trap->installed(), "Cannot verify the installed system-menu barrier");

        // This is the engine's logical input snapshot, not an OS-focus trace.
        // A hidden initialization can retain Application's true presence
        // defaults without receiving an activation/deactivation transition.
        app.keyboard()->poll();
        const bool initial_logical_focus = app.keyboard()->focused();
        app.keyboard()->poll();
        require(app.keyboard()->focused() == initial_logical_focus,
            "Engine logical keyboard focus changed during baseline polling");
        still_isolated(window, desktop);

        // Prove the trap consumes this command before any key can ask Windows
        // to generate it. This command is directed only to the owned HWND.
        SendMessageW(window, WM_SYSCOMMAND, SC_KEYMENU, 0);
        const Observation& guard = trap->observation(Phase::guard);
        require(guard.menu_requests == 1 && guard.zero_character_requests == 1 &&
            !trap->entered_menu_loop(), "The owned command barrier failed its self-test");
        still_isolated(window, desktop);

        send_pair(app, *trap, Phase::ordinary_a, WM_KEYDOWN, WM_KEYUP,
            'A', 0x1E, labrador::Key::a, false, desktop, initial_logical_focus);
        send_pair(app, *trap, Phase::f10, WM_SYSKEYDOWN, WM_SYSKEYUP,
            VK_F10, 0x44, labrador::Key::f10, false, desktop, initial_logical_focus);
        send_pair(app, *trap, Phase::bare_alt, WM_SYSKEYDOWN, WM_SYSKEYUP,
            VK_MENU, 0x38, labrador::Key::alt, true, desktop, initial_logical_focus);

        const Observation a = trap->observation(Phase::ordinary_a);
        const Observation f10 = trap->observation(Phase::f10);
        const Observation alt = trap->observation(Phase::bare_alt);
        require(a.menu_requests == 0 && !trap->entered_menu_loop(),
            "An ordinary key requested a menu or the trap allowed menu-loop entry");
        require(GetWindowLongPtrW(window, GWLP_USERDATA) == engine_user_data,
            "The owned tap changed the engine's window context");
        require(app.keyboard()->focused() == initial_logical_focus,
            "Engine logical keyboard focus changed before cleanup");
        still_isolated(window, desktop);
        require(trap->detach(), "Cannot restore the owned engine WndProc");
        ShowWindow(window, SW_HIDE);

        std::cout << "initial_logical_keyboard_focus=" << initial_logical_focus << '\n';
        std::cout << "phase,down_messages,up_messages,raw_down,raw_up,menu_requests,zero_character_requests\n";
        const auto row = [](const char* name, const Observation& value)
        {
            std::cout << name << ',' << value.down_messages << ',' << value.up_messages << ','
                << value.raw_down << ',' << value.raw_up << ',' << value.menu_requests << ','
                << value.zero_character_requests << '\n';
        };
        row("guard", guard); row("ordinary_a", a); row("f10", f10); row("bare_alt", alt);
        if (f10.menu_requests == 0 || alt.menu_requests == 0 ||
            f10.menu_requests != f10.zero_character_requests ||
            alt.menu_requests != alt.zero_character_requests)
        {
            std::cout << "NOT_REPRODUCED synthetic system keys: hidden delivery did not produce both bare SC_KEYMENU requests\n";
            return 2; // Do not activate the window or enter menu mode to force evidence.
        }
        std::cout << "REPRODUCED synthetic system keys: F10 and bare Alt reached engine Key snapshots and requested SC_KEYMENU; no menu-loop entry, global input or OS-focus-delivery claim\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
