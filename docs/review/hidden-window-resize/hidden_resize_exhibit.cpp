// Guarded Windows-only exhibit for hidden-window resize activation requests.
// Uses the real Labrador Window, no Application, renderer, audio or game assets.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "engine/app/window.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    // Installed before constructing our only window. The thread-local hook
    // vetoes activation/focus only for this process's windows on this thread.
    // It never permits an unguarded negative control or touches another app.
    class OwnedActivationVeto final
    {
    public:
        OwnedActivationVeto()
        {
            require(current_ == nullptr, "Activation veto must not nest");
            current_ = this;
            hook_ = SetWindowsHookExW(WH_CBT, &filter, nullptr, GetCurrentThreadId());
            if (hook_ == nullptr) { current_ = nullptr; }
            require(hook_ != nullptr, "Cannot install veto; no window was created");
        }
        ~OwnedActivationVeto()
        {
            current_ = nullptr;
            if (hook_ != nullptr) { UnhookWindowsHookEx(hook_); }
        }
        OwnedActivationVeto(const OwnedActivationVeto&) = delete;
        OwnedActivationVeto& operator=(const OwnedActivationVeto&) = delete;
        unsigned activation_attempts() const { return activation_attempts_; }
        unsigned focus_attempts() const { return focus_attempts_; }
    private:
        static LRESULT CALLBACK filter(int code, WPARAM word, LPARAM parameter)
        {
            if (code >= 0 && current_ != nullptr &&
                (code == HCBT_ACTIVATE || code == HCBT_SETFOCUS))
            {
                const HWND target = reinterpret_cast<HWND>(word);
                DWORD process = 0;
                const DWORD thread = GetWindowThreadProcessId(target, &process);
                if (thread == GetCurrentThreadId() && process == GetCurrentProcessId())
                {
                    if (code == HCBT_ACTIVATE) { ++current_->activation_attempts_; }
                    else { ++current_->focus_attempts_; }
                    return 1; // Observe the request, refuse it before activation.
                }
            }
            return CallNextHookEx(nullptr, code, word, parameter);
        }
        inline static thread_local OwnedActivationVeto* current_ = nullptr;
        HHOOK hook_ = nullptr;
        unsigned activation_attempts_ = 0;
        unsigned focus_attempts_ = 0;
    };

    class NoopNotify final : public labrador::WindowNotify
    {
    public:
        void tick() override {}
        void on_activated() override {}
        void on_deactivated() override {}
        void on_suspending() override {}
        void on_resuming() override {}
        void on_window_moved() const override {}
        void on_window_size_changed(int, int) override {}
        void on_key_down(labrador::Key) const override {}
        void on_key_up(labrador::Key) const override {}
        void on_text(char32_t) const override {}
        void on_mouse_move(int, int) const override {}
        void on_mouse_button_down(labrador::MouseButton) const override {}
        void on_mouse_button_up(labrador::MouseButton) const override {}
        void on_mouse_wheel(float) const override {}
        void on_mouse_wheel_horizontal(float) const override {}
        void on_touch_down(std::uint32_t, int, int) const override {}
        void on_touch_move(std::uint32_t, int, int) const override {}
        void on_touch_up(std::uint32_t, int, int) const override {}
        void on_touch_cancel(std::uint32_t) const override {}
    };

    void require_isolated(HWND window)
    {
        DWORD process = 0;
        require(GetWindowThreadProcessId(window, &process) == GetCurrentThreadId() &&
            process == GetCurrentProcessId(), "Window is not owned by this probe thread");
        if (IsWindowVisible(window) != FALSE)
        {
            ShowWindow(window, SW_HIDE); // Cleanup only our owned window.
            throw std::runtime_error("Probe window unexpectedly became visible");
        }
        require(GetActiveWindow() != window && GetForegroundWindow() != window &&
            GetFocus() != window, "Probe window unexpectedly gained activation or focus");
    }
}

int main()
{
    try
    {
        // Opaque read-only snapshot: never print, activate or modify this HWND.
        const HWND foreground_before = GetForegroundWindow();
        OwnedActivationVeto veto; // Outlives creation, resize and destruction.
        NoopNotify notify;
        unsigned attempts = 0, focus_attempts = 0;
        int width = 0, height = 0;
        bool foreground_unchanged_after_resize = false;
        {
            labrador::WindowOptions options;
            options.window_class_name = L"LabradorHiddenResizeExhibit";
            options.window_title = L"Hidden resize exhibit";
            options.client_size = { 640, 480 };
            options.visible = false;
            options.fullscreen = false;
            labrador::Window window(options, &notify);
            const HWND handle = static_cast<HWND>(window.handle());
            require_isolated(handle);
            require(veto.activation_attempts() == 0,
                "Construction unexpectedly requested activation; resize was not attempted");
            const unsigned activations = veto.activation_attempts();
            const unsigned focus = veto.focus_attempts();
            // Real engine call, continuously guarded; no loop or fullscreen.
            window.resize_client({ 800, 600 });
            require_isolated(handle);
            foreground_unchanged_after_resize =
                GetForegroundWindow() == foreground_before;
            RECT client = {};
            require(GetClientRect(handle, &client) != FALSE,
                "Cannot inspect the probe client area");
            width = client.right - client.left;
            height = client.bottom - client.top;
            attempts = veto.activation_attempts() - activations;
            focus_attempts = veto.focus_attempts() - focus;
        } // Window destroyed here while veto and notify are still alive.
        const bool foreground_unchanged_after_destruction =
            GetForegroundWindow() == foreground_before;
        const bool foreground_available = foreground_before != nullptr;
        const bool size_matches = width == 800 && height == 600;
        const char* outcome = !foreground_available ? "foreground-baseline-unavailable" :
            (!foreground_unchanged_after_resize || !foreground_unchanged_after_destruction)
                ? "desktop-foreground-changed" :
            !size_matches ? "client-size-mismatch" :
            attempts == 0 ? "no-activation-request-observed" : "activation-request-vetoed";
        const bool observed_attempt = foreground_available &&
            foreground_unchanged_after_resize && foreground_unchanged_after_destruction &&
            size_matches && attempts > 0;
        std::cout << std::boolalpha
            << "{\"scope\":\"guarded-native-window-resize\",\"outcome\":\"" << outcome
            << "\",\"activation_attempts\":" << attempts
            << ",\"focus_attempts\":" << focus_attempts
            << ",\"resize_isolated\":true,\"foreground_baseline_available\":" << foreground_available
            << ",\"foreground_unchanged_after_resize\":" << foreground_unchanged_after_resize
            << ",\"foreground_unchanged_after_destruction\":" << foreground_unchanged_after_destruction
            << ",\"client_size_matches\":" << size_matches
            << ",\"client_width\":" << width << ",\"client_height\":" << height
            << ",\"ungated_controls\":0}\n";
        // Focus requests may be positive independently. This outcome concerns
        // activation requests; no request or disturbed conditions are inconclusive.
        return observed_attempt ? 0 : 2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "SETUP_OR_ISOLATION_FAILURE " << error.what() << '\n';
        return 1;
    }
}