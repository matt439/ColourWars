#pragma once

// Test-only driver: callbacks inspect only this thread's exact owned dialog.
// Fixed storage and noexcept callbacks keep exceptions outside Win32 dispatch.
#include <array>
#include <cwchar>
#include <initializer_list>

namespace colourwars_editor
{
    namespace
    {
        struct PendingDialogStep
        {
            const wchar_t* title;
            const wchar_t* body;
            int answer;
            bool acknowledgement = false;
        };

        struct PendingDialogReceipt
        {
            HWND dialog = nullptr;
            int answer = 0;
            bool owned = false, title = false, body = false, button = false;
            bool hidden = false, attempted = false, posted = false, expired = false, gone = false;
            bool acknowledgement = false;
        };

        class PendingDialogs
        {
        public:
            explicit PendingDialogs(HWND owner) : owner_(owner), thread_(GetCurrentThreadId())
            {
                DWORD process = 0;
                require(active_ == nullptr && GetWindowThreadProcessId(owner_, &process) == thread_ &&
                    process == GetCurrentProcessId() && class_is(owner_, L"ColourWarsLevelEditor"),
                    "Pending dialog driver needs its exact private editor thread");
                active_ = this;
                timer_ = SetTimer(nullptr, 0, 20, timer);
                if (!timer_)
                {
                    active_ = nullptr;
                    throw std::runtime_error("Could not create the owned pending-property timer");
                }
            }
            ~PendingDialogs() { stop(); }
            PendingDialogs(const PendingDialogs&) = delete;
            PendingDialogs& operator=(const PendingDialogs&) = delete;

            void begin(std::initializer_list<PendingDialogStep> steps, bool expiry_control = false)
            {
                require(timer_ && !armed_ && steps.size() > 0 && steps.size() <= expected_.size(),
                    "Pending dialog driver needs one bounded nonnested action");
                expected_ = {}; receipts_ = {}; count_ = next_ = 0;
                unexpected_ = post_failed_ = overflow_ = expired_ = false;
                expiry_control_ = expiry_control;
                deadline_ = GetTickCount64() + (expiry_control ? 0 : 10000);
                for (const auto& step : steps) expected_[count_++] = step;
                armed_ = true;
            }

            bool finish() noexcept
            {
                advance();
                armed_ = false;
                bool valid = next_ == count_ && !unexpected_ && !post_failed_ && !overflow_ && timer_ &&
                    (expiry_control_ ? expired_ : !expired_);
                for (std::size_t index = 0; index < count_; ++index)
                {
                    const auto& receipt = receipts_[index];
                    valid = valid && receipt.owned && receipt.title && receipt.body && receipt.button && receipt.hidden &&
                        receipt.attempted && receipt.posted && receipt.gone &&
                        (expected_[index].acknowledgement && !expiry_control_ ?
                            receipt.acknowledgement && receipt.answer > 0 :
                            receipt.answer == (expiry_control_ ? IDCANCEL : expected_[index].answer));
                }
                return valid;
            }
            bool finish_fixture() noexcept { stop(); return killed_ && !has_dialog(); }
            bool running() const { return timer_ != 0; }
            const auto& receipts() const { return receipts_; }
            std::size_t count() const { return count_; }
            bool unexpected() const { return unexpected_; }
            bool expired() const { return expired_; }
            bool killed() const { return killed_; }

        private:
            static bool class_is(HWND window, const wchar_t* expected) noexcept
            {
                std::array<wchar_t, 64> name{};
                const int length = GetClassNameW(window, name.data(), static_cast<int>(name.size()));
                return length > 0 && length < static_cast<int>(name.size()) - 1 &&
                    std::wcscmp(name.data(), expected) == 0;
            }
            bool owned(HWND dialog) const noexcept
            {
                DWORD process = 0;
                return IsWindow(dialog) && GetWindowThreadProcessId(dialog, &process) == thread_ &&
                    process == GetCurrentProcessId() && GetWindow(dialog, GW_OWNER) == owner_ &&
                    class_is(dialog, L"#32770");
            }
            static HWND button(HWND dialog, int command) noexcept
            {
                const HWND child = GetDlgItem(dialog, command);
                return child && GetParent(child) == dialog && class_is(child, L"Button") &&
                    IsWindowEnabled(child) ? child : nullptr;
            }
            static HWND acknowledgement_button(HWND dialog) noexcept
            {
                HWND result = nullptr;
                HWND child = GetWindow(dialog, GW_CHILD);
                std::size_t visited = 0;
                for (; child && visited < 128; child = GetWindow(child, GW_HWNDNEXT), ++visited)
                {
                    if (GetParent(child) != dialog || !class_is(child, L"Button") ||
                        !IsWindowEnabled(child) || !IsWindowVisible(child)) continue;
                    if (result) return nullptr;
                    const int identifier = GetDlgCtrlID(child);
                    if (identifier <= 0 || identifier > 0xffff) return nullptr;
                    result = child;
                }
                return child ? nullptr : result;
            }
            static BOOL CALLBACK body_child(HWND child, LPARAM parameter) noexcept
            {
                auto& match = *reinterpret_cast<BodyMatch*>(parameter);
                if (GetParent(child) != match.dialog || !class_is(child, L"Static")) return TRUE;
                std::array<wchar_t, 512> text{};
                const int length = GetWindowTextW(child, text.data(), static_cast<int>(text.size()));
                if (length > 0 && length < static_cast<int>(text.size()) - 1 && std::wcscmp(text.data(), match.expected) == 0)
                    match.found = true;
                return TRUE;
            }
            struct BodyMatch { HWND dialog; const wchar_t* expected; bool found = false; };
            static bool body_is(HWND dialog, const wchar_t* expected) noexcept
            {
                BodyMatch match{ dialog, expected };
                EnumChildWindows(dialog, body_child, reinterpret_cast<LPARAM>(&match));
                return match.found;
            }
            static bool title_is(HWND dialog, const wchar_t* expected) noexcept
            {
                std::array<wchar_t, 128> title{};
                const int length = GetWindowTextW(dialog, title.data(), static_cast<int>(title.size()));
                return length > 0 && length < static_cast<int>(title.size()) - 1 && std::wcscmp(title.data(), expected) == 0;
            }
            bool identity(HWND dialog, const PendingDialogStep& step) const noexcept
            { return owned(dialog) && title_is(dialog, step.title) && body_is(dialog, step.body); }
            void post(HWND dialog, HWND control, int command) noexcept
            {
                auto& receipt = receipts_[next_];
                receipt.answer = command;
                receipt.attempted = true;
                receipt.posted = PostMessageW(dialog, WM_COMMAND, MAKEWPARAM(command, BN_CLICKED),
                    reinterpret_cast<LPARAM>(control)) != FALSE;
                post_failed_ = post_failed_ || !receipt.posted;
            }
            static BOOL CALLBACK inspect(HWND dialog, LPARAM parameter) noexcept
            {
                auto& driver = *reinterpret_cast<PendingDialogs*>(parameter);
                if (!driver.owned(dialog) || !IsWindowVisible(dialog)) return TRUE;
                if (driver.next_ >= driver.count_) { driver.overflow_ = true; return TRUE; }
                auto& receipt = driver.receipts_[driver.next_];
                if (receipt.attempted) return TRUE;
                receipt.dialog = dialog; receipt.owned = true;
                receipt.title = title_is(dialog, driver.expected_[driver.next_].title);
                receipt.body = body_is(dialog, driver.expected_[driver.next_].body);
                const bool exact = receipt.title && receipt.body && !IsWindowEnabled(driver.owner_);
                const bool deadline = GetTickCount64() >= driver.deadline_;
                if (!exact) driver.unexpected_ = true;
                if (deadline) driver.expired_ = true;
                // An unexpected dialog is never answered affirmatively. A
                // verified owned Cancel button is the only fallback action.
                int answer = exact && !deadline ? driver.expected_[driver.next_].answer : IDCANCEL;
                const bool acknowledge = exact && !deadline && driver.expected_[driver.next_].acknowledgement;
                // MB_OK defines the returned result, not the system template's
                // native button ID. Only this exact error may use its sole
                // enabled acknowledgement; Yes and Cancel retain strict IDs.
                const HWND control = acknowledge ? acknowledgement_button(dialog) : button(dialog, answer);
                if (acknowledge && control) answer = GetDlgCtrlID(control);
                receipt.acknowledgement = acknowledge && control != nullptr;
                receipt.button = control != nullptr; receipt.expired = deadline;
                if (!control) return TRUE;
                ShowWindow(dialog, SW_HIDE);
                receipt.hidden = !IsWindowVisible(dialog);
                driver.post(dialog, control, answer);
                return TRUE;
            }
            void advance() noexcept
            {
                // A subsequent message box may reuse the old HWND. Its full
                // caption/body identity, rather than handle existence, ends a step.
                if (next_ < count_ && receipts_[next_].posted && !identity(receipts_[next_].dialog, expected_[next_]))
                { receipts_[next_].gone = true; ++next_; }
            }
            static void CALLBACK timer(HWND, UINT message, UINT_PTR identifier, DWORD) noexcept
            {
                PendingDialogs* driver = active_;
                if (!driver || !driver->armed_ || message != WM_TIMER || identifier != driver->timer_ ||
                    GetCurrentThreadId() != driver->thread_) return;
                driver->advance();
                if (driver->next_ < driver->count_)
                    EnumThreadWindows(driver->thread_, inspect, reinterpret_cast<LPARAM>(driver));
            }
            void stop() noexcept
            {
                // KillTimer leaves queued callbacks behind. Remove their
                // context first. One timer exists for the whole fixture, so
                // action transitions never reuse timer identifiers or contexts.
                if (active_ == this) active_ = nullptr;
                armed_ = false;
                if (timer_)
                { killed_ = KillTimer(nullptr, timer_) != FALSE; timer_ = 0; }
            }
            static BOOL CALLBACK find_dialog(HWND dialog, LPARAM parameter) noexcept
            {
                auto& driver = *reinterpret_cast<PendingDialogs*>(parameter);
                if (driver.owned(dialog)) { driver.remaining_dialog_ = true; return FALSE; }
                return TRUE;
            }
            bool has_dialog() noexcept
            {
                remaining_dialog_ = false;
                EnumThreadWindows(thread_, find_dialog, reinterpret_cast<LPARAM>(this));
                return remaining_dialog_;
            }
            inline static thread_local PendingDialogs* active_ = nullptr;
            HWND owner_ = nullptr;
            DWORD thread_ = 0;
            ULONGLONG deadline_ = 0;
            UINT_PTR timer_ = 0;
            std::array<PendingDialogStep, 2> expected_{};
            std::array<PendingDialogReceipt, 2> receipts_{};
            std::size_t count_ = 0, next_ = 0;
            bool expiry_control_ = false, expired_ = false, unexpected_ = false;
            bool post_failed_ = false, overflow_ = false, killed_ = false;
            bool armed_ = false, remaining_dialog_ = false;
        };
    }
}
